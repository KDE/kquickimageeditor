// SPDX-FileCopyrightText: 2010 Mario Klingemann <mario@quasimondo.com>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "stackblur.h"

#include "utils.h"

#include <QImage>
#include <QtConcurrent/QtConcurrentMap>

#include <hwy/aligned_allocator.h>
#include <hwy/base.h> // basic helper macros, typedefs, concepts and functions

using namespace UtilsNS;
using namespace StackBlur;

// Prevent multiple inclusion when <hwy/foreach_target.h> re-includes this file.
#ifndef HELPERS_DEFINED
#define HELPERS_DEFINED

namespace StackBlur
{
/*
In this file, we try to express the meaning of variable types using aliases.
This way we don't end up with a soup of hard to distinguish integers and floats.
Besides the type aliases we define below, we use size_t to express a value that
is supposed to be based on memory units or locations in memory. Example:
`size_t yBytes` represents the number of bytes between pixels on the Y axis.
It can be added to uint8_t image data pointers to get a Y axis pixel pointer.

In KDE, we generally don't like pointer arithmetic or raw pointers, but it's
very common for image processing. I've tried using STL ranges, spans and custom
span classes to make it more like modern C++, but it's far more trouble than
it's worth. The advantage (and disadvantage) of pointer arithmetic is that it's easier to jump around in arbitrary intervals. I can jump to the next pixel on
the X or Y axis, jump to the next channel, the same channel on the next pixel,
etc., without ever needing to change types or switch to a different range/view.
It's also much easier to preserve things like restricted pointers and alignment
assumptions to help the compiler optimize things as much as possible.
*/

// Pixel coordinate type
using Px = decltype(std::declval<QImage>().width());

template<typename T>
concept ChannelType = std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>;

// Metadata for threaded blurring
template<ChannelType C_t, size_t Channels, size_t Alignment>
    requires ChannelCount<Channels> && IsPow2<Alignment>
struct BlurTask {
    using Channel_t = C_t;
    using Pixel_t = hwy::UnsignedFromSize<sizeof(Channel_t) * Channels>; // pixel type
    static constexpr auto channels = Channels;
    static constexpr auto alignment = Alignment;
    Channel_t *data = nullptr;
    size_t stride = 0;
    Px width = 0;
    Px radius = 0;
    Px startRow = 0;
    Px endRow = 0;
};

template<typename T>
concept BlurTaskType = std::is_same_v<T, BlurTask<typename T::Channel_t, T::channels, T::alignment>>;

// Metadata for threaded transposing
template<ChannelType C_t, size_t Channels, size_t Alignment>
    requires ChannelCount<Channels> && IsPow2<Alignment>
struct TransposeTask {
    using Channel_t = C_t;
    using Pixel_t = hwy::UnsignedFromSize<sizeof(Channel_t) * Channels>; // pixel type
    static constexpr auto channels = Channels;
    static constexpr auto alignment = Alignment;
    const Channel_t *srcData = nullptr;
    Channel_t *dstData = nullptr;
    size_t srcStride = 0;
    size_t dstStride = 0;
    Px width = 0; // Width of the source matrix
    Px height = 0; // Height of the source matrix
    Px startTileY = 0;
    Px endTileY = 0;
    Px tileWidth = 0;
    Px tileHeight = 0;
};

template<typename T>
concept TransposeTaskType = std::is_same_v<T, TransposeTask<typename T::Channel_t, T::channels, T::alignment>>;

template<typename MathType>
constexpr MathType radiusMultiplier(Px radius)
{
    return MathType{1} / ((radius + MathType{1}) * (radius + MathType{1}));
}

// We need a concise and 100% always inlined way to declare a restricted pointer
// with alignment assumptions. If it was a constexpr or inline function and the
// compiler decided not to inline it, the restriction and alignment assumptions
// would be lost. We use HWY_ASSUME_ALIGNED instead of std::assume_aligned since
// <hwy/base.h> docs say MSVC's __assume is unsuitable. HWY_ASSUME_ALIGNED just
// gives the pointer when using MSVC. We use a C style cast instead of C++ style
// static_cast or reinterpret_cast because MSVC doesn't compile with static_cast
// and reinterpret_cast in this case.
#define GET_OPTIMIZED_PTR(UnderlyingType, ptr, align) (UnderlyingType * HWY_RESTRICT)(HWY_ASSUME_ALIGNED(ptr, align))
#define DECL_OPTIMIZED_PTR(UnderlyingType, varName, ptr, align) UnderlyingType *HWY_RESTRICT varName = GET_OPTIMIZED_PTR(UnderlyingType, ptr, align)

// Used to enable/disable features specifically for the benchmark
#ifdef STACKBLURBENCHMARK
static constexpr bool s_isBenchmarkBuild = true;
#else
static constexpr bool s_isBenchmarkBuild = false;
#endif

// Environment variable purely for use with the benchmark. Not for apps.
static const Px s_envTileSize = []() -> Px {
    if constexpr (!s_isBenchmarkBuild) {
        return 0;
    }
    auto tileSize = qEnvironmentVariableIntValue("STACKBLURBENCHMARK_TILESIZE");
    // 512 to 4096 are really only for benchmarking.
    // They're pretty bad for performance.
    // Round to the next power of 2 down.
    return std::bit_floor(static_cast<size_t>(std::clamp(tileSize, 0, 4096)));
}();

} // namespace StackBlur
#endif // HELPERS_DEFINED

// HWY_TARGET_INCLUDE must be defined as the file with the code to be copied for
// each target before including <hwy/foreach_target.h>.
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "stackblur.cpp"
// <hwy/foreach_target.h> is a special header that copies the code from
// HWY_TARGET_INCLUDE for each target. Ensure that only code that should be
// copied is copied using `#ifndef #define MY_DEFINITION [code here…] #endif`
// guards or `#if HWY_ONCE [code here…] #endif`.
// HWY_ONCE can only be used once.
#include <hwy/foreach_target.h>
// <hwy/foreach_target.h> must be included before <hwy/highway.h>
#include <hwy/highway.h>

// Set up SIMD code for each target in the following HWY_NAMESPACE.
// From Highway docs: If you choose not to use the BEFORE/AFTER lines, you must
// prefix any function that calls Highway ops such as Load with HWY_ATTR. Either
// of these will set the compiler #pragma required to generate vector code.
// Lambda functions currently require HWY_ATTR before their opening brace.
HWY_BEFORE_NAMESPACE();
// HWY_NAMESPACE must be nested in the project's namespace.
// All the stuff in here slows down compile times, so consider keeping stuff
// that doesn't need to be here out of here. Templates can also be problematic
// if there are too many code branches.
namespace StackBlur::HWY_NAMESPACE
{

/* hwy::HWY_NAMESPACE is basically a baseline static target namespace used to
 * access the Highway SIMD APIs. Their implementation may be replaced with the
 * target implementation at runtime via HWY_EXPORT* and HWY_DYNAMIC_* macros.
 *
 * NOTE: You can't use HWY_EXPORT on functions that have `noexcept` or have
 * types defined within this scope in their signature.
 *
 * The baseline static target depends on how Highway was compiled. On my Linux
 * disribution, it's N_SSE2, which means the distro targets SSE2 as a lowest
 * common denominator for X86-64 CPUs instead of choosing the target that is
 * fastest for my specific CPU (a common practice).
 */

namespace hn = hwy::HWY_NAMESPACE;

/* A quick note on the difference between multi-threading and SIMD:
- Multi-threading is like a leader delegating tasks to subordinates. The leader
  won't know exactly how long the subordinates will take to finish tasks or the
  order in which they will finish. A leader should make sure their subordinates
  can accomplish their tasks without getting in each other's way and make sure
  their subordinates aren't given more tasks than they can efficiently handle
  at one time.
- SIMD is like using a multi-barreled shotgun that shoots all shells at once.
  You have to be mindful about how you load and fire the shells to be efficient
  and safe. Make sure they're the right size and make sure you don't load too
  many at once. If you try to load more shells than you own (e.g., indexes past
  the end of your image's data array), that's stealing and it's illegal (crash).

It's best to try to work in groups of 128 bits or 256 bits since those are
the sizes for SIMD registers. We don't do that since it was too hard for
me to find a way to maximize throughput for a stack blur.

You can do arithmetic on whole pixels or uniformly sized groups of pixels
without getting channel values wrong. The main issue with types for whole
pixels is that it's harder to prevent overflows when doing math on larger
types. uint32 (uint8 * 4 channels) is already at the edge of what you can
still use for math since the only standard type that supports even larger
values is 64-bit. uint64 (uint16 * 4 channels) is too big to multiply and
add to. uint128 exists (usually provided by a library), but Highway can't
use it for Highway tags. It likely would not be optimized anyway. Because
of all that, we treat pixels as groups of channels instead of using types
that could be treated as whole pixels. Whole pixel types are still OK for
assignment and copying when we aren't using SIMD vectors.

CPU designers put a lot of effort into making arithmetic with floats as
fast as possible over the past 10 years, even to the point of neglecting
other types. Because of that, floats have become the most optimal type for
arithmetic. They're not good for indexes or bit shifting. int32 and uint32
are the most optimal for those.

It's a shame that uint16 is not one of the optimal types for arithmetic
since we could match a vector of 16 uint8 channels (128 bits) with a
vector of 16 uint16 channels (256 bits).
*/

static_assert(
    []() constexpr -> bool {
        using C_t = uint16_t; // channel type
        constexpr size_t channels = 4;
        constexpr size_t RGBA64_alignment = alignof(C_t) * channels;
        using Tag = hn::FixedTag<C_t, channels>;
        constexpr size_t maxLanes = HWY_MAX_LANES_D(Tag);
        return IsSufficientlyAligned<RGBA64_alignment, alignof(hn::TFromD<Tag>) * maxLanes>;
    }(),
    "We expect RGBA64 to have a minimum alignment like this on x86-64");
static_assert(
    []() constexpr -> bool {
        using i386_size_t = uint32_t;
        using C_t = uint16_t;
        constexpr i386_size_t channels = 4;
        constexpr i386_size_t i386_RGBA64_alignment = alignof(i386_size_t);
        using Tag = hn::FixedTag<C_t, channels>;
        constexpr i386_size_t maxLanes = HWY_MAX_LANES_D(Tag);
        return !IsSufficientlyAligned<i386_RGBA64_alignment, alignof(hn::TFromD<Tag>) * maxLanes>;
    }(),
    "We expect RGBA64 to be potentially insufficiently aligned like this on i386");

template<size_t Alignment, typename FromTag, typename InterTag, typename ToTag>
    requires IsPow2<Alignment> // Can't really do requirements for tags
HWY_ATTR HWY_API hn::Vec<ToTag> loadPtrToVec(FromTag from, InterTag inter, ToTag to, const hn::TFromD<FromTag> *HWY_RESTRICT ptr)
{
    using FromT = hn::TFromD<FromTag>;
    // Load is for aligned data. LoadU is for potentially unaligned data.
    if constexpr (IsSufficientlyAligned<Alignment, alignof(FromT) * HWY_MAX_LANES_D(FromTag)>) {
        // To go from uint8/uint16 to float32, you have to go to int32 first.
        return hn::ConvertTo(to, hn::PromoteTo(inter, hn::Load(from, GET_OPTIMIZED_PTR(FromT, ptr, Alignment))));
    } else {
        return hn::ConvertTo(to, hn::PromoteTo(inter, hn::LoadU(from, ptr)));
    }
};

template<size_t Alignment, typename FromVec, typename InterTag, typename ToTag>
    requires IsPow2<Alignment> // Can't really do requirements for tags
HWY_ATTR HWY_API void storeVecToPtr(FromVec from, InterTag inter, ToTag to, hn::TFromD<ToTag> *HWY_RESTRICT ptr)
{
    using ToT = hn::TFromD<ToTag>;
    // Store is for aligned data. StoreU is for potentially unaligned data.
    if constexpr (IsSufficientlyAligned<Alignment, alignof(typename ToTag::T) * HWY_MAX_LANES_D(ToTag)>) {
        // To go from float32 to uint8/uint16, you have to go to int32 first.
        hn::Store(hn::DemoteTo(to, hn::ConvertTo(inter, from)), to, GET_OPTIMIZED_PTR(ToT, ptr, Alignment));
    } else {
        hn::StoreU(hn::DemoteTo(to, hn::ConvertTo(inter, from)), to, ptr);
    }
};

/*
Stack blur: Row major pass, meaning `for (y…) {for (x…) {…}}`.
Images are technically 1D dynamic arrays with metadata like stride and height
to help you interpret them in 2D. The X axis runs directly along the 1D array
and the Y axis requires a stride multiplier to get to the next Y position up
or down. Since the outer loop is for Y and the inner loop is for X, this
strides less and accesses sequential data more often, which is more cache
friendly. However, you have O(kernelSize) active states to manage.
*/
template<BlurTaskType BlurTask>
// HWY_FLATTEN tells the compiler to inline functions used by this function
HWY_ATTR HWY_FLATTEN void blurRowWorker(const BlurTask &task)
{
    static constexpr auto channels = BlurTask::channels;
    static constexpr auto alignment = BlurTask::alignment;

    using C_t = typename BlurTask::Channel_t; // channel type
    using M_t = hwy::float32_t; // math type;
    using I_t = hwy::MakeSigned<M_t>; // intermediary type
    static_assert(sizeof(hwy::MakeWide<C_t>) <= sizeof(M_t),
                  "The math type should be large enough that it won't overflow when doing arithmetic with channels.");
    // Rebind makes a similar tag with a different type and the same number of lanes
    using MTag = hn::FixedTag<M_t, channels>; // math tag type
    using CTag = hn::Rebind<C_t, MTag>; // channel tag type
    using ITag = hn::Rebind<I_t, MTag>; // intermediary tag type
    // Highway can't go directly from uint8/uint16 to float32, so we need to
    // convert to the intermediary type first.
    // We have separate asserts for each condition because static_assert errors
    // can look vague when you put too many tests in one static_assert.
    // We could put all these asserts in a test file, but I think it's good to
    // verify our assumptions where we are making them. If it's tucked away in
    // some test file, they won't be as effective for giving the reader context
    // for how the code should work.
    static_assert(requires { hn::PromoteTo(ITag(), hn::Vec<CTag>()); }, "Ensure we can promote from channel to intermediary type");
    static_assert(requires { hn::DemoteTo(CTag(), hn::Vec<ITag>()); }, "Ensure we can demote from intermediary to channel type");
    static_assert(requires { hn::ConvertTo(MTag(), hn::Vec<ITag>()); }, "Ensure we can convert from intermediary to math type");
    static_assert(requires { hn::ConvertTo(ITag(), hn::Vec<MTag>()); }, "Ensure we can convert from math to intermediary type");
    // Vectors are what actually hold data, but they don't have any members.
    // They don't even have a size until you zero them or assign something.
    // You have to use Highway's APIs to do anything with them.

    // Tags don't have data, Highway just uses them to pass type info.
    MTag mtag;
    ITag itag;
    CTag ctag;

    const Px width = task.width;
    const Px lastX = width - 1; // last X pos index
    const Px radius = task.radius;
    const Px kernelSize = kernelSizeFromRadius(radius);

    // "End" in the same way that std::end is after the last index.
    // "Start" in the same way that std::begin is the first index.
    // Can be used for kernel or X pos loop.
    const Px leftEndMidStart = std::min(radius + 1, width);
    // These are only for the X pos loop.
    const Px midBlockKernelMidX = leftEndMidStart + radius;
    const Px midBlockEndRightEdgeStart = std::max(width - radius - 1, leftEndMidStart);

    // A whole vector of the same multiplier. It seem wasteful, but we have to
    // do this to multiply it with another vector.
    const auto vMultiplier = hn::Set(mtag, radiusMultiplier<M_t>(radius));

    // Can't put SIMD vectors in a struct/class or any container type on ARM
    // with SVE since the SIMD vector sizes are only known at runtime.

    // Weights should be set like {1, 2, 3, …, radius+1, …, 3, 2, 1}
    hwy::AlignedVector<M_t> weights(kernelSize);
    // set left side of kernel weights
    for (Px i = 0; i < leftEndMidStart; ++i) {
        weights[i] = i + 1;
    }
    // set middle and right side of kernel weights
    for (Px i = leftEndMidStart; i < kernelSize; ++i) {
        weights[i] = kernelSize - i;
    }

    // A queue of channel data used by the kernel
    hwy::AlignedVector<M_t> stack(kernelSize * channels);

    for (Px y = task.startRow; y < task.endRow; ++y) {
        // Inform the compiler if the pointer is aligned and restricted
        DECL_OPTIMIZED_PTR(C_t, rowData, task.data + y * task.stride, alignment);

        /*
        The kernel is basically like this on the X axis:
        after <- sumOut <- stackSum/X pos <- sumIn <- before
        We move right until the end, then we repeat on the next Y position.
        stackSum has sumOut subtracted and sumIn added on every iteration.
        The influence of each pixel is like a quantized bell curve shape.
        This is why stack blur looks kind of like a Gaussian (bell curve) blur.
        */
        auto sumIn = hn::Zero(mtag); // sum for right/incoming
        auto stackSum = hn::Zero(mtag); // sum of middle+right-left
        auto sumOut = hn::Zero(mtag); // sum for left/outgoing

        const auto vFirst = loadPtrToVec<alignment>(ctag, itag, mtag, &rowData[0]);

        // fill up left side of kernel
        for (Px i = 0; i < leftEndMidStart; ++i) {
            std::copy_n(&rowData[0], channels, &stack[i * channels]);
            // MulAdd multiplies the first two args, then adds the last.
            // Sometimes it's more optimal than Add(Mul(v0, v1), v2).
            stackSum = hn::MulAdd(vFirst, hn::Set(mtag, weights[i]), stackSum);
            sumOut = hn::Add(sumOut, vFirst);
        }
        // fill up middle and right side of kernel
        for (Px i = leftEndMidStart; i < kernelSize; ++i) {
            const Px nextX = std::min(i - radius, lastX); // starts at 1
            const auto vNext = loadPtrToVec<alignment>(ctag, itag, mtag, &rowData[nextX * channels]);
            std::copy_n(&rowData[nextX * channels], channels, &stack[i * channels]);
            stackSum = hn::MulAdd(vNext, hn::Set(mtag, weights[i]), stackSum);
            sumIn = hn::Add(sumIn, vNext);
        }

        // We split the left, middle and right sections of X positions to reduce
        // the amount of branching caused by bounds checking on the X axis.
        enum Sections : uint8_t {
            LeftEdge,
            MiddleBlock,
            RightEdge,
        };
        auto forX = [&]<Sections Section>(Px kernelMidX, Px startX, Px endX) HWY_ATTR HWY_FLATTEN {
            // We need to re-declare every time to ensure we get our restriction
            // and alignment back.
            DECL_OPTIMIZED_PTR(C_t, row, rowData, alignment);
            for (Px x = startX; x < endX; ++x) {
                const auto blurred = hn::Mul(stackSum, vMultiplier);
                storeVecToPtr<alignment>(blurred, itag, ctag, &row[x * channels]);
                stackSum = hn::Sub(stackSum, sumOut);
                // Outgoing data kernel index.
                // The modulo lets us loop through the kernel.
                const Px outKI = (kernelMidX - radius + kernelSize) % kernelSize;
                sumOut = hn::Sub(sumOut, hn::Load(mtag, &stack[outKI * channels]));
                // Next middle of kernel
                if constexpr (Section == LeftEdge) {
                    kernelMidX = std::min(kernelMidX + 1, lastX);
                } else if constexpr (Section == MiddleBlock) {
                    ++kernelMidX;
                }
                const auto vNextKMid = loadPtrToVec<alignment>(ctag, itag, mtag, &row[kernelMidX * channels]);
                // outgoing is replaced by next
                std::copy_n(&row[kernelMidX * channels], channels, &stack[outKI * channels]);
                sumIn = hn::Add(sumIn, vNextKMid);
                stackSum = hn::Add(stackSum, sumIn);
                const auto vCurrentKMid = hn::Load(mtag, &stack[(kernelMidX % kernelSize) * channels]);
                sumOut = hn::Add(sumOut, vCurrentKMid);
                sumIn = hn::Sub(sumIn, vCurrentKMid);
            }
        };
        forX.template operator()<LeftEdge>(radius, 0, leftEndMidStart);
        forX.template operator()<MiddleBlock>(midBlockKernelMidX, leftEndMidStart, midBlockEndRightEdgeStart);
        forX.template operator()<RightEdge>(lastX, midBlockEndRightEdgeStart, width);
    }
}

// Copy the source image rotated 90 degrees
template<TransposeTaskType TransposeTask>
HWY_ATTR HWY_FLATTEN void transposeWorker(const TransposeTask &task)
{
    static constexpr auto channels = TransposeTask::channels;
    static constexpr auto alignment = TransposeTask::alignment;
    using P_t = typename TransposeTask::Pixel_t;
    for (Px y = task.startTileY; y < task.endTileY; y += task.tileHeight) {
        const Px yMax = std::min(y + task.tileHeight, task.height);
        for (Px x = 0; x < task.width; x += task.tileWidth) {
            const Px xMax = std::min(x + task.tileWidth, task.width);
            for (Px ty = y; ty < yMax; ++ty) {
                for (Px tx = x; tx < xMax; ++tx) {
                    DECL_OPTIMIZED_PTR(const P_t, srcData, task.srcData + ty * task.srcStride + tx * channels, alignment);
                    DECL_OPTIMIZED_PTR(P_t, dstData, task.dstData + tx * task.dstStride + ty * channels, alignment);
                    *dstData = *srcData;
                }
            }
        }
    }
}

} // namespace StackBlur::HWY_NAMESPACE
HWY_AFTER_NAMESPACE(); // Required to finish the SIMD code setup

// Prevent multiple inclusion in each Highway target. Can only be used once.
#if HWY_ONCE

namespace StackBlur
{
template<ChannelType C_t, size_t Channels, size_t Alignment>
    requires ChannelCount<Channels> && IsPow2<Alignment>
void exec(QImage &image, Px radiusX, Px radiusY)
{
    using BlurTask = BlurTask<C_t, Channels, Alignment>;
    using BlurTaskList = QList<BlurTask>;
    using TransposeTask = TransposeTask<C_t, Channels, Alignment>;
    using TransposeTaskList = QList<TransposeTask>;
    const Px w = image.width();
    const Px h = image.height();
    const size_t strideElements = w * Channels;
    C_t *data = reinterpret_cast<C_t *>(image.bits());
    // `HWY_EXPORT_T` has you set a function table name and a function.
    // It is how you export template functions, but it only works with
    // a single template. If you try `myFunc<Arg1, Arg2>`, it won't work.
    // You also can't export `noexcept` functions.
    HWY_EXPORT_T(blurRowWorker, blurRowWorker<BlurTask>);
    HWY_EXPORT_T(transposeWorker, transposeWorker<TransposeTask>);

    /*
    The CPU is faster at processing image data in tiles.

    We only iterate over tiles when transposing image data because the blur
    needs to access all neighboring pixels on the same row. We still split
    the rows into chunks by tiles for blurring so that the threads are split
    in a more cache friendly way.

    This logic is an attempt to scale tile size with the amount of data
    being processed. It's hard to figure out a way to calculate this
    with a linear equation. I've tried using quantities of bytes, but
    performance doesn't necessarily scale with bytes as you'd think.
    For example, larger tile sizes are worse for processing very large
    images (fairly straight forward relationship). However, if a format
    has fewer and smaller channels (e.g., Alpha8), its speed still scales
    with tile size (in pixel positions) similarly to larger formats like
    ARGB32 or RGBA64. Then there are times when larger tiles slow down
    Alpha8 and speed up RGBA64 or the opposite happens. Sometimes ARGB32.
    is the one that slows down while RGBA64 becomes even faster than ARGB32.
    It also depends on the hardware. For reference, I was testing with an
    AMD Ryzen 7 4800H CPU. This is the best I could come up with.
    When I speak of ARGB32 and RGBA64, they are the premultiplied variants.
    If you want to do your own testing, use the STACKBLURBENCHMARK_TILESIZE
    and STACKBLURBENCHMARK_RESOLUTION environment variables and the
    -iterations CLI option when running stackblurbenchmark_bin manually.

    I've also tried longer and wider tiles, but they didn't really improve
    anything.
    */
    const Px tileSize = [w] {
        if constexpr (s_isBenchmarkBuild) {
            if (s_envTileSize) {
                return s_envTileSize;
            }
        }
        // Above somewhere around this size, 64 becomes slower than 32.
        // We use width for the threshold because height is split by threads
        // while width is not. Sometimes tall images are easier to deal with
        // even if they're less cache friendly.
        if (w <= 8192) {
            // This works fine for all formats and sizes below the threshold
            return 64;
        }
        // This works fine for all formats and sizes above the threshold
        return 32;
    }();

    if (radiusX >= 1) {
        const auto blurTasks = makeTasks<BlurTaskList>(h, tileSize, [&](BlurTaskList &tasks, Px start, Px end) {
            tasks.push_back({data, strideElements, w, radiusX, start, end});
        });
        // HWY_DYNAMIC_POINTER_T is actually the same as HWY_DYNAMIC_POINTER
        // but it has slightly different semantics since the arg name is
        // TABLE_NAME.
        maybeBlockingMap(blurTasks, HWY_DYNAMIC_POINTER_T(blurRowWorker));
    }

    if (radiusY >= 1) {
        // Transpose to make the vertical pass more cache friendly.
        // It's also easier to split threads by height than width while
        // preventing sections of memory from being shared between threads.
        // Transposing turns width into height, so it solves that issue too.
        // NOTE: An RGBA64 screenshot of three 4K monitors will need
        // 199'065'600 bytes, which is kind of a lot.
        hwy::AlignedVector<C_t> transBuffer(w * h * Channels);
        const size_t transStrideElements = h * Channels;

        auto transTasks = makeTasks<TransposeTaskList>(h, tileSize, [&](TransposeTaskList &tasks, Px start, Px end) {
            tasks.push_back({data, transBuffer.data(), strideElements, transStrideElements, w, h, start, end, tileSize, tileSize});
        });
        maybeBlockingMap(transTasks, HWY_DYNAMIC_POINTER_T(transposeWorker));

        // Blur the transposed image
        const auto blurTasks = makeTasks<BlurTaskList>(w, tileSize, [&](BlurTaskList &tasks, Px start, Px end) {
            tasks.push_back({transBuffer.data(), transStrideElements, h, radiusY, start, end});
        });
        maybeBlockingMap(blurTasks, HWY_DYNAMIC_POINTER_T(blurRowWorker));

        // Untranspose
        transTasks = makeTasks<TransposeTaskList>(w, tileSize, [&](TransposeTaskList &tasks, Px start, Px end) {
            tasks.push_back({transBuffer.data(), data, transStrideElements, strideElements, h, w, start, end, tileSize, tileSize});
        });
        maybeBlockingMap(transTasks, HWY_DYNAMIC_POINTER_T(transposeWorker));
    }
}

void blur(QImage &image, Px radiusX, Px radiusY)
{
    static constexpr auto supportedFormats = supportedImageFormats();
    // Null images will have Format_Invalid. Images with (width < 1 || height < 1)
    // will be null, so this also checks if the size is valid.
    if (std::find(supportedFormats.begin(), supportedFormats.end(), image.format()) == supportedFormats.end()) {
        return;
    }
    // Radius is never more than (size-1)/2 because the kernel should always
    // have an odd size and should never be bigger than the image.
    radiusX = std::min({radiusX, (image.width() - 1) / 2, maxRadius()});
    radiusY = std::min({radiusY, (image.height() - 1) / 2, maxRadius()});
    if (radiusX < minRadius() && radiusY < minRadius()) {
        return;
    }

    // Never use copy captures that copy the QImage (e.g., [=], [image, …])
    // for these lambdas. If you do, the QImage will be copied and the next time
    // we use `QImage &` or call a QImage function that could modify its data,
    // the QImage will "detach" and all of the underlying data will be copied.

    auto execAlignment = [&]<ChannelType C_t, size_t Channels, size_t PixelAlignment>
        requires ChannelCount<Channels> && IsPow2<PixelAlignment> && IsPow2<HWY_ALIGNMENT>
    {
        static_assert(std::is_same_v<decltype(image), QImage &>, "Don't copy the QImage. Otherwise, you'll detach it again.");
        // Knowing alignment allows us to help the compiler make optimizations.

        // QImage::constScanLine docs:
        // The scanline data is as minimum 32-bit aligned. For 64-bit
        // formats it follows the native alignment of 64-bit integers
        // (64-bit for most platforms, but notably 32-bit on i386).
        static constexpr size_t minAlignment = alignof(uint32_t);
        // size_t is 32bit on i386.
        static constexpr size_t formatAlignment = std::clamp(PixelAlignment, minAlignment, alignof(size_t));
        // <hwy/aligned_allocator.h> HWY_ALIGNMENT docs:
        // Minimum alignment of allocated memory for use in HWY_ASSUME_ALIGNED, which
        // requires a literal. To prevent false sharing, this should be at least the
        // L1 cache line size, usually 64 bytes. However, Intel's L2 prefetchers may
        // access pairs of lines, and M1 L2 and POWER8 lines are also 128 bytes.
        static constexpr size_t minL2CacheAlignment = std::max(size_t{HWY_ALIGNMENT}, formatAlignment);
        static constexpr size_t minL1CacheAlignment = std::max(64uz, formatAlignment);

        // `image.bits()`, `image.constBits()`, `image.scanLine(0)`
        // and `image.constScanLine(0)` all return the same thing.
        const auto data = reinterpret_cast<const C_t *>(image.constBits());
        if (hwy::IsAligned(data, minL2CacheAlignment)) {
            exec<C_t, Channels, minL2CacheAlignment>(image, radiusX, radiusY);
        } else if (hwy::IsAligned(data, minL1CacheAlignment)) {
            exec<C_t, Channels, minL1CacheAlignment>(image, radiusX, radiusY);
        } else if (hwy::IsAligned(data, formatAlignment)) {
            exec<C_t, Channels, formatAlignment>(image, radiusX, radiusY);
        } else {
            exec<C_t, Channels, minAlignment>(image, radiusX, radiusY);
        }
    };
    const auto pixelFormat = image.pixelFormat();
    const auto channelCount = pixelFormat.channelCount();
    const auto bytesPerChannel = (pixelFormat.bitsPerPixel() / channelCount) >> 3;
    auto execChannelCount = [&]<ChannelType C_t>() {
        // We only support 1 or 4 channels right now since Alpha8
        // and common RGB formats have 1 and 4 channels.
        if (channelCount == 1) {
            execAlignment.operator()<C_t, 1, alignof(C_t) * 1>();
        } else if (channelCount == 4) {
            execAlignment.operator()<C_t, 4, alignof(C_t) * 4>();
        }
    };
    if (bytesPerChannel == sizeof(uint8_t)) {
        execChannelCount.operator()<uint8_t>();
    } else if (bytesPerChannel == sizeof(uint16_t)) {
        execChannelCount.operator()<uint16_t>();
    }
    // No support for floats or 32-bit (or higher) integer per channel formats.
}

} // namespace StackBlur
#endif // HWY_ONCE

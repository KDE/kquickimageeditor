// SPDX-FileCopyrightText: 2010 Mario Klingemann <mario@quasimondo.com>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "stackblur.h"

#include <QImage>
#include <QtConcurrent/QtConcurrentMap>

// Includes basic helper macros, typedefs, concepts and functions
#include <hwy/base.h>

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

template<size_t Channels>
concept ChannelCount = Channels == 1 || Channels == 4;

template<auto V>
concept IsPow2 = (std::is_integral_v<decltype(V)> || std::is_enum_v<decltype(V)>) && (std::has_single_bit(static_cast<size_t>(V)));

// Used to check if lambdas/functions are compatible with other functions.
// Does not do strict type checking.
template<typename Func, typename Ret, typename... Args>
concept CompatibleSignature = std::invocable<Func, Args...> // check args
    && std::convertible_to<std::invoke_result_t<Func, Args...>, Ret>; // check return type

template<size_t Alignment, size_t RequiredAlignment>
concept IsSufficientlyAligned = IsPow2<Alignment>
    && IsPow2<RequiredAlignment>
    // A cheaper modulo that only works when the right side is a power of 2.
    && (Alignment & (RequiredAlignment - 1)) == 0;

// Helps you split an extent into chunks based on the size of the chunks.
template<std::integral T>
constexpr auto extentToChunks(T extent, T chunk)
{
    return (extent + chunk - 1) / chunk;
}

// Allows us to create a list of tasks with the option to have slight variations
// from other lists of tasks using the pushBackFunction argument.
template<std::ranges::contiguous_range Container, std::integral Extent, typename Function>
    requires CompatibleSignature<Function, void, Container &, Extent, Extent>
inline Container makeTasks(Extent extent, Extent chunkSize, Function pushBackFunction)
{
    // Thread count can change over time, so get a new thread count every time.
    const Extent threadCount = QThread::idealThreadCount();
    const Extent extentPerThread = extentToChunks(extent, chunkSize * threadCount);
    Container tasks;
    for (Extent i = 0; i < threadCount; ++i) {
        const Extent start = i * chunkSize * extentPerThread;
        const Extent end = std::min(start + chunkSize * extentPerThread, extent);
        if (start < end) {
            pushBackFunction(tasks, start, end);
        }
    }
    return tasks;
}

// Avoids needing to use blockingMap if we only have 1 task.
inline void maybeBlockingMap(const auto &tasks, auto function)
{
    if (tasks.size() == 1) {
        function(tasks.front());
        return;
    }
    QtConcurrent::blockingMap(tasks, function);
}

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

} // namespace StackBlur::HWY_NAMESPACE
HWY_AFTER_NAMESPACE(); // Required to finish the SIMD code setup

// Prevent multiple inclusion in each Highway target. Can only be used once.
#if HWY_ONCE

namespace StackBlur
{

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
}

} // namespace StackBlur
#endif // HWY_ONCE

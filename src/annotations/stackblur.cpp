// SPDX-FileCopyrightText: 2010 Mario Klingemann <mario@quasimondo.com>
// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "stackblur.h"

#include <QImage>
#include <QtConcurrent/QtConcurrentMap>

using namespace StackBlur;

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

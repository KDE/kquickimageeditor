# SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

add_executable(stackblurtest_bin
    stackblurtest.cpp
    ../src/annotations/QtCV.h
)
target_include_directories(stackblurtest_bin PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(stackblurtest_bin Qt::Test Qt::Gui ${OpenCV_LIBRARIES})
kde_target_enable_exceptions(stackblurtest_bin PRIVATE)
ecm_mark_as_test(stackblurtest_bin)

# Run the benchmarks with just 1 iteration during CI, so we known it works
add_test(NAME stackblurtest COMMAND stackblurtest_bin "-iterations" "10")

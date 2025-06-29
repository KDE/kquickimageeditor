# SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

add_executable(stackblurtest_bin
    stackblurtest.cpp
    ../src/annotations/stackblur.cpp
)
target_link_libraries(stackblurtest_bin Qt::Test Qt::Gui)
ecm_mark_as_test(stackblurtest_bin)

# Run the benchmarks with just 1 iteration during CI, so we known it works
add_test(NAME stackblurtest COMMAND stackblurtest_bin "-iterations" "10")

if (OpenCV_DIR)
    add_executable(stackbluropencvtest_bin
        stackblurtest.cpp
        ../src/annotations/stackblur_opencv.cpp
    )
    kde_target_enable_exceptions(stackbluropencvtest_bin PRIVATE)
    target_include_directories(stackbluropencvtest_bin PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(stackbluropencvtest_bin Qt::Test Qt::Gui ${OpenCV_LIBRARIES})
    ecm_mark_as_test(stackbluropencvtest_bin)

    # Run the benchmarks with just 1 iteration during CI, so we known it works
    add_test(NAME stackbluropencvtest COMMAND stackbluropencvtest_bin "-iterations" "10")
endif()

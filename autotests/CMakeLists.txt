# SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

add_executable(stackblurtest_bin
    stackblurtest_helpers.h
    stackblurtest.cpp
    ../src/annotations/stackblur.cpp
)
# needed to compile with Highway's dynamic dispatch
target_include_directories(stackblurtest_bin PRIVATE ../src/annotations)
target_link_libraries(stackblurtest_bin PRIVATE Qt::Test Qt::Gui Qt::Concurrent hwy::hwy)
ecm_mark_as_test(stackblurtest_bin)
add_test(NAME stackblurtest COMMAND stackblurtest_bin)
# Timeout after 10 seconds in case it hangs.
set_tests_properties(stackblurtest PROPERTIES TIMEOUT 10)

add_executable(stackblurbenchmark_bin
    stackblurtest_helpers.h
    stackblurbenchmark.cpp
    ../src/annotations/stackblur.cpp
)
# needed to compile with Highway's dynamic dispatch
target_include_directories(stackblurbenchmark_bin PRIVATE ../src/annotations)
target_link_libraries(stackblurbenchmark_bin PRIVATE Qt::Test Qt::Gui Qt::Concurrent hwy::hwy)
# Enable benchmark specific features
target_compile_definitions(stackblurbenchmark_bin PRIVATE STACKBLURBENCHMARK)
ecm_mark_as_test(stackblurbenchmark_bin)
# Run the benchmarks with just 1 iteration during CI, so we known it works
# Run the binary manually if you want to do a heavier test.
add_test(NAME stackblurbenchmark_smoketest COMMAND stackblurbenchmark_bin "-iterations" "1")
# Timeout after 10 seconds in case it hangs.
# Limit smoketest variables to small sizes to keep it fast.
set_tests_properties(stackblurbenchmark_smoketest PROPERTIES TIMEOUT 10 ENVIRONMENT "STACKBLURBENCHMARK_RESOLUTION=min;STACKBLURBENCHMARK_RADIUS=min")

add_executable(coloradjustmenttest_bin
    stackblurtest_helpers.h
    coloradjustmenttest.cpp
    ../src/annotations/coloradjustment.cpp
)
target_include_directories(coloradjustmenttest_bin PRIVATE ../src/annotations)
target_link_libraries(coloradjustmenttest_bin PRIVATE Qt::Test Qt::Gui)
ecm_mark_as_test(coloradjustmenttest_bin)
add_test(NAME coloradjustmenttest COMMAND coloradjustmenttest_bin)
# Timeout after 10 seconds in case it hangs.
set_tests_properties(stackblurtest PROPERTIES TIMEOUT 10)

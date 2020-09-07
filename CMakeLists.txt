# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.5)

set(KF5_VERSION "5.70.0") # handled by release scripts
set(KF5_DEP_VERSION "5.70.0") # handled by release scripts
set(REQUIRED_QT_VERSION 5.12.0)

project(KQuickImageEditor LANGUAGES CXX VERSION ${KF5_VERSION})

find_package(ECM ${KF5_VERSION} NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(GNUInstallDirs)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

add_subdirectory(src)
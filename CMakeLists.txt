# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

set(REQUIRED_KF_VERSION 6.0.0)
set(REQUIRED_QT_VERSION 6.6.0)
set(KDE_COMPILERSETTINGS_LEVEL 5.84.0)

project(KQuickImageEditor LANGUAGES CXX VERSION 0.5.1)

find_package(ECM ${REQUIRED_KF_VERSION} NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)
include(ECMSetupVersion)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMGenerateQmlTypes)
include(CMakePackageConfigHelpers)
include(KDEPackageAppTemplates)
include(ECMQmlModule)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KQuickImageEditor")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KQuickImageEditorConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfig.cmake"
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_PREFIX
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfig.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KQUICKIMAGEEDITOR
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kquickimageeditor_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfigVersion.cmake"
    SOVERSION 1
)


find_package(Qt6 ${REQUIRED_QT_VERSION} COMPONENTS Core Quick REQUIRED)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

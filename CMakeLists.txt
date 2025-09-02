# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

set(REQUIRED_KF_VERSION 6.13.0)
set(REQUIRED_QT_VERSION 6.8.0)

project(KQuickImageEditor LANGUAGES CXX VERSION 0.5.1)

find_package(ECM ${REQUIRED_KF_VERSION} NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(FeatureSummary)
include(CMakePackageConfigHelpers)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(KDEPackageAppTemplates)

include(ECMAddTests)
include(ECMSetupVersion)
include(ECMGenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGenerateQmlTypes)
include(ECMQmlModule)

find_package(Qt6 ${REQUIRED_QT_VERSION} COMPONENTS Core Qml Quick REQUIRED)
find_package(KF6 ${REQUIRED_KF_VERSION} REQUIRED Config)

if(NOT WIN32)
    find_package(OpenCV COMPONENTS core imgproc)
    if (NOT OpenCV_DIR)
        message("OpenCV is missing. Build with OpenCV to get better blur performance.")
    endif()
endif()

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KQuickImageEditor")
set(KQuickImageEditor_INSTALL_INCLUDEDIR ${KDE_INSTALL_INCLUDEDIR}/KQuickImageEditor)
set(kquickimageeditor_INSTALL_INCLUDEDIR ${KDE_INSTALL_INCLUDEDIR}/kquickimageeditor)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KQUICKIMAGEEDITOR
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kquickimageeditor_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfigVersion.cmake"
    SOVERSION 1
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KQuickImageEditorConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfig.cmake"
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_PREFIX
)

add_subdirectory(src)
if (BUILD_TESTING)
  add_subdirectory(autotests)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kquickimageeditor_version.h DESTINATION ${KQuickImageEditor_INSTALL_INCLUDEDIR} COMPONENT Devel)
install(EXPORT KQuickImageEditorTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KQuickImageEditorTargets.cmake)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfig.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

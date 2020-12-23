# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.5)

set(KF5_VERSION "5.70.0")
set(REQUIRED_QT_VERSION 5.15.0)

project(KQuickImageEditor LANGUAGES CXX VERSION 0.1.2)

find_package(ECM ${KF5_VERSION} NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(GNUInstallDirs)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)
include(ECMPoQmTools)
include(ECMSetupVersion)
include(CMakePackageConfigHelpers)
#include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDEPackageAppTemplates)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KQuickImageEditor")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KQuickImageEditorConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KQuickImageEditorConfig.cmake"
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS  KF5_INCLUDE_INSTALL_DIR CMAKE_INSTALL_PREFIX
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


find_package(Qt5 ${REQUIRED_QT_VERSION} COMPONENTS Core Quick REQUIRED)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

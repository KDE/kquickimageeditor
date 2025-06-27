# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
# SPDX-FileCopyrightText: 2025 Noah Davis <noahadvs@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

add_library(KQuickImageEditor)

ecm_add_qml_module(KQuickImageEditor
    URI "org.kde.kquickimageeditor"
    CLASS_NAME KQuickImageEditorPlugin
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    INSTALLED_PLUGIN_TARGET KQuickImageEditorPlugin
    DEPENDENCIES
        "QtQuick"
        "QtQuick.Controls"
        "QtQuick.Effects"
        "QtQuick.Shapes"
        "QtQuick.Templates"
        "org.kde.kirigami"
)

ecm_generate_export_header(KQuickImageEditor
    VERSION ${PROJECT_VERSION}
    BASE_NAME KQuickImageEditor
)
set_target_properties(KQuickImageEditor PROPERTIES
    VERSION     ${PROJECT_VERSION}
    SOVERSION   1
    EXPORT_NAME KQuickImageEditor
)

kconfig_target_kcfg_file(KQuickImageEditor
    FILE annotationconfig.kcfg
    CLASS_NAME AnnotationConfig
    DEFAULT_VALUE_GETTERS
    GENERATE_MOC
    GENERATE_PROPERTIES
    MUTATORS
    QML_REGISTRATION
    SINGLETON
)

target_sources(KQuickImageEditor PRIVATE
    annotations/annotationdocument.cpp
    annotations/annotationdocument.h
    annotations/annotationdocument_p.h
    annotations/annotationtool.cpp
    annotations/annotationtool.h
    annotations/annotationviewport.cpp
    annotations/annotationviewport.h
    annotations/history.cpp
    annotations/history.h
    annotations/qmlpainterpath.cpp
    annotations/qmlpainterpath.h
    annotations/stackblur.cpp
    annotations/stackblur.h
    annotations/traits.cpp
    annotations/traits.h
    annotations/utils.h

    commands/undocommand.cpp
    commands/undocommand.h
    commands/cropcommand.cpp
    commands/cropcommand.h
    commands/resizecommand.cpp
    commands/resizecommand.h
    commands/mirrorcommand.cpp
    commands/mirrorcommand.h
    commands/rotatecommand.cpp
    commands/rotatecommand.h
    resizehandle.cpp
    resizehandle.h
    resizerectangle.cpp
    resizerectangle.h
    imageitem.cpp
    imageitem.h
    imagedocument.cpp
    imagedocument.h
)

ecm_target_qml_sources(KQuickImageEditor SOURCES
    annotations/AnnotationSelectionTool.qml
    annotations/CropTool.qml
    annotations/HoverOutline.qml
    annotations/TextTool.qml
    controls/BasicResizeHandle.qml
    controls/CropBackground.qml
    controls/RectangleCutout.qml
    controls/SelectionBackground.qml
    controls/SelectionHandle.qml
    controls/SelectionTool.qml
)

ecm_target_qml_sources(KQuickImageEditor PRIVATE PATH private SOURCES
    annotations/private/AnimatedLoader.qml
    annotations/private/DashedOutline.qml
    annotations/private/Handle.qml
    annotations/private/Outline.qml
    annotations/private/TextContextMenu.qml
)

kde_target_enable_exceptions(KQuickImageEditor PRIVATE)
target_link_libraries(KQuickImageEditor
    PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    KF6::ConfigCore
    KF6::ConfigGui
)
target_include_directories(KQuickImageEditor PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/annotations>")
target_include_directories(KQuickImageEditor INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KQuickImageEditor>")

ecm_finalize_qml_module(KQuickImageEditor DESTINATION ${KDE_INSTALL_QMLDIR} EXPORT KQuickImageEditorTargets)

ecm_generate_headers(KQuickImageEditor_FORWARDING_HEADERS
    HEADER_NAMES
        AnnotationDocument
        AnnotationTool
        AnnotationViewport
    PREFIX KQuickImageEditor
    REQUIRED_HEADERS KQuickImageEditor_HEADERS
    RELATIVE annotations
)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KQuickImageEditor LIB_NAME KQuickImageEditor DEPS "core qml quick" FILENAME_VAR PRI_FILENAME )

install(TARGETS KQuickImageEditor EXPORT KQuickImageEditorTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${KQuickImageEditor_FORWARDING_HEADERS} DESTINATION ${KQuickImageEditor_INSTALL_INCLUDEDIR} COMPONENT Devel)
install(
    FILES
        ${KQuickImageEditor_HEADERS}
        ${CMAKE_CURRENT_BINARY_DIR}/kquickimageeditor_export.h
    DESTINATION ${kquickimageeditor_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

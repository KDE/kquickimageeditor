# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(kquickimageeditorplugin URI "org.kde.kquickimageeditor" VERSION 1.0 GENERATE_PLUGIN_SOURCE DEPENDENCIES "QtQuick" "QtQuick.Effects")

target_sources(kquickimageeditorplugin PRIVATE
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
    ${kquickimageeditor_QM_LOADER}
)

ecm_target_qml_sources(kquickimageeditorplugin SOURCES
    controls/BasicResizeHandle.qml
    controls/SelectionTool.qml
    controls/SelectionHandle.qml
    controls/SelectionBackground.qml
    controls/CropBackground.qml
    controls/RectangleCutout.qml
)

target_link_libraries(kquickimageeditorplugin
    PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
)

ecm_finalize_qml_module(kquickimageeditorplugin DESTINATION ${KDE_INSTALL_QMLDIR})

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KQuickImageEditor LIB_NAME KQuickImageEditor DEPS "core qml quick svg" FILENAME_VAR PRI_FILENAME )
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

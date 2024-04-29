# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(kquickimageeditorplugin URI "org.kde.kquickimageeditor" VERSION 1.0)

target_sources(kquickimageeditorplugin PRIVATE
    code/commands/undocommand.cpp
    code/commands/undocommand.h
    code/commands/cropcommand.cpp
    code/commands/cropcommand.h
    code/commands/resizecommand.cpp
    code/commands/resizecommand.h
    code/commands/mirrorcommand.cpp
    code/commands/mirrorcommand.h
    code/commands/rotatecommand.cpp
    code/commands/rotatecommand.h
    code/kquickimageeditor_plugin.cpp
    code/kquickimageeditor_plugin.h
    code/resizehandle.cpp
    code/resizehandle.h
    code/resizerectangle.cpp
    code/resizerectangle.h
    code/imageitem.cpp
    code/imageitem.h
    code/imagedocument.cpp
    code/imagedocument.h
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

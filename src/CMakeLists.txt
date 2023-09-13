# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

if(QT_MAJOR_VERSION STREQUAL "5")
	set(QML_QTGRAPHICAL_EFFECTS_IMPORT "QtGraphicalEffects 1.15")
else()
	set(QML_QTGRAPHICAL_EFFECTS_IMPORT "Qt5Compat.GraphicalEffects")
endif()

configure_file(controls/SelectionHandle.qml ${CMAKE_CURRENT_BINARY_DIR}/controls/SelectionHandle.qml)

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
    # ${CMAKE_CURRENT_BINARY_DIR}/controls/SelectionHandle.qml
    controls/SelectionBackground.qml
    controls/CropBackground.qml
    controls/RectangleCutout.qml
)

target_link_libraries(kquickimageeditorplugin
    PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Quick
    Qt${QT_MAJOR_VERSION}::Qml
)

ecm_finalize_qml_module(kquickimageeditorplugin DESTINATION ${KDE_INSTALL_QMLDIR})

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KQuickImageEditor LIB_NAME KQuickImageEditor DEPS "core qml quick svg" FILENAME_VAR PRI_FILENAME )
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

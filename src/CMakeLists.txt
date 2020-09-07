# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

include_directories(code)

set(
    sources
    
    code/commands/undocommand.cpp
    code/commands/cropcommand.cpp
    code/commands/mirrorcommand.cpp
    code/commands/rotatecommand.cpp
    code/kquickimageeditor_plugin.cpp
    code/resizehandle.cpp
    code/resizerectangle.cpp
    code/imageitem.cpp
    code/imagedocument.cpp
    resources.qrc
)
set(
    pluginData

    qrc/qmldir
)

add_library(
    kquickimageeditorplugin
    SHARED

    ${sources} ${pluginData}
)

target_compile_options(
    kquickimageeditorplugin
    PRIVATE
        -Werror
)

target_link_libraries(
    kquickimageeditorplugin
    PRIVATE
        Qt5::Core
        Qt5::Quick
        Qt5::Qml
)

add_custom_command(
    TARGET kquickimageeditorplugin
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_LIST_DIR}/qrc/qmldir
        $<TARGET_FILE_DIR:kquickimageeditorplugin>/qrc/qmldir
)

install(FILES qrc/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor)
install(TARGETS kquickimageeditorplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor )
install(DIRECTORY qrc/controls DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor)
# SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

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
    controls/qmldir
    ${kquickimageeditor_QM_LOADER}
)

ecm_generate_qmltypes(org.kde.kquickimageeditor 1.0
                      DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor)

add_library(
    kquickimageeditorplugin
    SHARED
    ${sources}
)

target_link_libraries(
    kquickimageeditorplugin
    PRIVATE
        Qt5::Core
        Qt5::Quick
        Qt5::Qml
)

install(TARGETS kquickimageeditorplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor )

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

if (BUILD_SHARED_LIBS)
    add_custom_target(copy_to_bin ALL
        COMMAND ${CMAKE_COMMAND} -E
                make_directory ${CMAKE_BINARY_DIR}/bin/org/kde/kquickimageeditor/
        COMMAND ${CMAKE_COMMAND} -E
                copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/controls ${CMAKE_BINARY_DIR}/bin/org/kde/kquickimageeditor/
        COMMAND ${CMAKE_COMMAND} -E
                copy $<TARGET_FILE:kquickimageeditorplugin> ${CMAKE_BINARY_DIR}/bin/org/kde/kquickimageeditor/
    )

    install(DIRECTORY controls/ DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor)

    include(ECMGeneratePriFile)
    ecm_generate_pri_file(BASE_NAME KQuickImageEditor LIB_NAME KQuickImageEditor DEPS "core qml quick svg" FILENAME_VAR PRI_FILENAME )
    install(FILES ${PRI_FILENAME}
            DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

endif()

install(TARGETS kquickimageeditorplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kquickimageeditor )

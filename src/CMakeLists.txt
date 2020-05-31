include_directories(code)

set(
    sources

    code/kquickimageeditor_plugin.cpp
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
        -Wall -Werror
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
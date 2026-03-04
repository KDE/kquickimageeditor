# SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause


ecm_add_qml_module(KQuickImageEditorUI
    URI "org.kde.kquickimageeditor.ui"
    CLASS_NAME KQuickImageEditorUIPlugin
    GENERATE_PLUGIN_SOURCE
    INSTALLED_PLUGIN_TARGET KQuickImageEditorUIPlugin
    DEPENDENCIES
        "org.kde.kquickimageeditor"
        "org.kde.ki18n"
    SOURCES
        filedialoghelper.cpp
        filedialoghelper.h
)
ecm_generate_qdoc(KQuickImageEditorUI kquickimageeditorui.qdocconf)

ecm_target_qml_sources(KQuickImageEditorUI SOURCES
	ImageEditorPage.qml
)

ecm_target_qml_sources(KQuickImageEditorUI PRIVATE PATH private SOURCES
    private/AnimatedLoader.qml
    private/AnnotationEditor.qml
    private/AnnotationOptionsToolBarContents.qml
    private/AnnotationsToolBarContents.qml
    private/ButtonGrid.qml
    private/ConfirmDiscardingChanges.qml
    private/ImageView.qml
)

target_link_libraries(KQuickImageEditorUI PRIVATE
    Qt6::Quick
    Qt6::Qml
)

ecm_finalize_qml_module(KQuickImageEditorUI DESTINATION ${KDE_INSTALL_QMLDIR} EXPORT KQuickImageEditorTargets)

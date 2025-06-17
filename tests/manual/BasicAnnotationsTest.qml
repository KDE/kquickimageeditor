import QtQuick
import org.kde.kquickimageeditor

AnnotationEditor {
    id: root
    width: 1024
    height: 768
    Shortcut {
        sequences: ["Ctrl+1"]
        onActivated: root.document.tool.type = AnnotationTool.NoTool
    }
    Shortcut {
        sequences: ["Ctrl+2"]
        onActivated: root.document.tool.type = AnnotationTool.SelectTool
    }
    Shortcut {
        sequences: ["Ctrl+3"]
        onActivated: root.document.tool.type = AnnotationTool.FreehandTool
    }
    Shortcut {
        sequences: ["Ctrl+4"]
        onActivated: root.document.tool.type = AnnotationTool.HighlighterTool
    }
    Shortcut {
        sequences: ["Ctrl+5"]
        onActivated: root.document.tool.type = AnnotationTool.LineTool
    }
    Shortcut {
        sequences: ["Ctrl+6"]
        onActivated: root.document.tool.type = AnnotationTool.ArrowTool
    }
    Shortcut {
        sequences: ["Ctrl+7"]
        onActivated: root.document.tool.type = AnnotationTool.RectangleTool
    }
    Shortcut {
        sequences: ["Ctrl+8"]
        onActivated: root.document.tool.type = AnnotationTool.EllipseTool
    }
    Shortcut {
        sequences: ["Ctrl+9"]
        onActivated: root.document.tool.type = AnnotationTool.BlurTool
    }
    Shortcut {
        sequences: ["Ctrl+0"]
        onActivated: root.document.tool.type = AnnotationTool.PixelateTool
    }
    Shortcut {
        sequences: ["Ctrl+-"]
        onActivated: root.document.tool.type = AnnotationTool.TextTool
    }
    Shortcut {
        sequences: ["Ctrl+="]
        onActivated: root.document.tool.type = AnnotationTool.NumberTool
    }
}

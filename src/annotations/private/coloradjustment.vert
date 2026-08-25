// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;
layout(location = 0) out vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    mat4 colorMatrix;
    float gamma;
    float qt_Opacity;
};

out gl_PerVertex { vec4 gl_Position; };

void main() {
    texCoord = qt_MultiTexCoord0;
    gl_Position = qt_Matrix * qt_Vertex;
}

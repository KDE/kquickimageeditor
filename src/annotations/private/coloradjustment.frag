// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

vec3 unpremultiply(vec3 color, float alpha)
{
    return color / alpha;
}

vec3 premultiply(vec3 color, float alpha)
{
    return color * alpha;
}

vec3 linearColor(vec3 color) {
    bvec3 linearThreshold = lessThanEqual(color, vec3(0.04045f));
    return mix(pow((color + vec3(0.055f)) / 1.055f, vec3(2.4f)),
               color / 12.92f,
               linearThreshold);
}

vec3 sRGBColor(vec3 color) {
    bvec3 linearThreshold = lessThanEqual(color, vec3(0.0031308f));
    return mix(pow(color, vec3(1.0f / 2.4f)) * 1.055f - vec3(0.055f),
               color * 12.92f,
               linearThreshold);
}

vec3 mapMat4Vec3(mat4 mat, vec3 color)
{
    vec4 mapped = mat * vec4(color, 1.0f);
    return clamp(mapped.rgb / mapped.w, 0.0f, 1.0f);
}

vec3 adjustGamma(vec3 color, float gamma)
{
    return pow(color, vec3(gamma));
}

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    mat4 colorMatrix;
    float gamma;
    float qt_Opacity;
};

layout(binding = 1) uniform sampler2D source;

void main() {
    fragColor = texture(source, texCoord);
    if (fragColor.a == 0) {
        return;
    }
    vec3 adjustedColor = unpremultiply(fragColor.rgb, fragColor.a);
    adjustedColor = linearColor(adjustedColor);
    adjustedColor = mapMat4Vec3(colorMatrix, adjustedColor);
    adjustedColor = adjustGamma(adjustedColor, gamma);
    adjustedColor = sRGBColor(adjustedColor);
    fragColor.rgb = premultiply(adjustedColor, fragColor.a);
    fragColor = fragColor * qt_Opacity;
}

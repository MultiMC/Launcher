#version 150 core

/* Copyright 2025 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

out vec4 finalColor;

uniform vec3 baseColor;
uniform vec3 alternateColor;
uniform float fDevicePixelSize;

void main() {
    float fieldSize = fDevicePixelSize * 25.0;
    if ((int(floor(gl_FragCoord.x / fieldSize) + floor(gl_FragCoord.y / fieldSize)) & 1) == 0)
        finalColor = vec4(baseColor, 1.0);
    else
        finalColor = vec4(alternateColor, 1.0);
}

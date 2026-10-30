#version 150 core

/* Copyright 2025 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

// vertex shader

in vec3 position;
in vec2 texcoords;
// floats, because Qt 5 can only feed attributes through glVertexAttribPointer
in float texnr;
in float transparency;

out vec2 texCoord;
flat out int texID;
flat out int texTransparency;

uniform mat4 worldToView;

void main() {
    gl_Position = worldToView * vec4(position, 1.0);
    texCoord = texcoords;
    texID = int(texnr + 0.5);
    texTransparency = int(transparency + 0.5);
}

// Copyright 2023 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

//? #version 430 core

in vec2 frag_tex_coord;
out vec4 color;

uniform sampler2D color_texture;

uniform vec4 i_resolution;
uniform vec4 o_resolution;
uniform int layer;

void main() {
    color = texture(color_texture, frag_tex_coord);
}

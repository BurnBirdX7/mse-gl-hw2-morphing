#version 330 core

uniform sampler2D tex_2d;

in vec3 vert_col;
in vec2 vert_tex;

out vec4 out_col;

void main() {
    out_col = texture(tex_2d, vert_tex);
}
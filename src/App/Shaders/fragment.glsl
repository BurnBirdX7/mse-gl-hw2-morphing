#version 330 core

uniform sampler2D tex_2d;

in vec3 fragNormal;
in vec2 fragTex;
in vec3 fragPos;
in vec3 fragDirectionalLightSource;

out vec4 outCol;

void main() {
    vec4 directional = vec4(0); // Default value

    float cos = dot(fragNormal, fragDirectionalLightSource);
    if (cos > 0.0) {
        directional = texture(tex_2d, fragTex) * cos;
    } else {
        directional = vec4(0);
    }

    outCol = texture(tex_2d, fragTex) * 0.8 + directional;
}
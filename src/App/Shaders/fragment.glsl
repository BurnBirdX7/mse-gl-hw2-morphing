#version 330 core

uniform sampler2D tex_2d;

in vec3 fragNormal;
in vec2 fragTex;
in vec3 fragPos;
in vec3 fragDirectionalLightSource;
in vec3 fragSpotLightDirection;

out vec4 outCol;

void main() {
    vec4 tex = texture(tex_2d, fragTex);

    float ambient = 0.2;
    float directional = max(dot(fragNormal, fragDirectionalLightSource), 0);

    outCol = (ambient + directional) * tex;
}
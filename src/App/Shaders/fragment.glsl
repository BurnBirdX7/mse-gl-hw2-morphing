#version 330 core

uniform sampler2D tex_2d;

in vec3 fragNormal;
in vec2 fragTex;
in vec3 fragPos;                        // World space
in vec3 fragDirectionalLightSource;     // View space
in vec3 fragSpotLightDirection;         // View space
in vec3 fragSpotLightRelativePosition;  // View space

out vec4 outCol;

vec4 get_spot() {
    vec4 spotLightColor = vec4(0.6, 0.6, 0.9, 1);
    float cos = dot(normalize(fragSpotLightRelativePosition), fragSpotLightDirection);
    if (cos > 0 && acos(cos) < radians(20.0)) {
        return spotLightColor;
    }
    return vec4(0);
}


void main() {
    // Constants:

    float ambient = 0.2;
    vec4 tex = texture(tex_2d, fragTex);
    float directional = max(dot(fragNormal, fragDirectionalLightSource), 0);
    vec4 spot = get_spot();


    outCol = (ambient + directional + spot) * tex;
}
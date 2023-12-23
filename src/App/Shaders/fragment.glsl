#version 330 core

uniform sampler2D tex_2d;

in vec3 fragNormal;
in vec2 fragTex;
in vec3 fragPos;                        // View space
in vec3 fragSpotLightDirection;         // View space
in vec3 fragSpotLightRelativePosition;  // View space

out vec4 outColor;

vec4 get_spot() {
    const vec4 spotLightColor = vec4(0.6, 0.6, 0.9, 1); // RGBA color
    float cos = dot(normalize(fragSpotLightRelativePosition), fragSpotLightDirection);
    if (cos > 0 && acos(cos) < radians(20.0)) {
        return spotLightColor;
    }
    return vec4(0);
}

vec4 get_directional() {
    const vec4 diffuseLightColor = vec4(1.0, 0.92, 0.5, 1.0); // RGBA Color
    const vec3 diffuseLightSource = vec3(3.0, 5.0, 1.0);      // Position in World space
    vec3 diffuseLightDirection = normalize(diffuseLightSource - fragPos);
    return max(dot(fragNormal, diffuseLightDirection), 0) * diffuseLightColor;
}

vec4 get_ambient() {
    const vec4 ambientLightColor = vec4(1.0, 1.0, 1.0, 1.0);
    const float ambientStength = 0.2;
    return ambientLightColor * ambientStength;
}

void main() {
    // Constants:

    vec4 tex = texture(tex_2d, fragTex);
    vec4 directional = get_directional();
    vec4 spot = get_spot();
    vec4 ambient = get_ambient();

    outColor = (ambient + directional + spot) * tex;
}
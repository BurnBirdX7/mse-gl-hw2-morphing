#version 330 core

layout(location=0) in vec3 vertVertex;
layout(location=1) in vec3 vertNormal;
layout(location=2) in vec2 vertTex;

// Matrices:
uniform mat4 viewMat;
uniform mat4 mvp;
uniform mat4 modelMat;
uniform mat4 normalMat;

// Params:
uniform float sphereMorph;
uniform vec3 spotLightSource;
uniform vec3 spotLightDirection;

// Output:
out vec3 fragNormal;
out vec2 fragTex;
out vec3 fragPos;					    // World space
out vec3 fragSpotLightDirection;	    // View space
out vec3 fragSpotLightRelativePosition; // View space

float get_scale() {
	vec3 scaleFactors = vec3(
		length(modelMat[0].xyz),
		length(modelMat[1].xyz),
		length(modelMat[2].xyz)
	);
	return 1 / max(max(scaleFactors.x, scaleFactors.y), scaleFactors.z);
}

void main() {
	// Vertex:
	vec4 vertex = vec4(mix(vertVertex, get_scale() * normalize(vertVertex), sphereMorph), 1.0);
	vec3 normal = mix(vertNormal, normalize(vertVertex), sphereMorph);

	// Output:
	fragPos = vec3(modelMat * vertex);
	fragNormal = normalize(mat3(normalMat) * normal);
	fragTex = vertTex;

	// Spot light
	fragSpotLightDirection = normalize(mat3(viewMat) * (spotLightDirection - spotLightSource));
	fragSpotLightRelativePosition = vec3(viewMat * (vec4(fragPos, 1) - vec4(spotLightSource, 1)));

	// Postion output
	gl_Position = mvp * vertex;
}

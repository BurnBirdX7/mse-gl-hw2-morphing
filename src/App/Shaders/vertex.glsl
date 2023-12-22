#version 330 core

layout(location=0) in vec3 vertVertex;
layout(location=1) in vec3 vertNormal;
layout(location=2) in vec2 vertTex;

uniform mat4 mvp;
uniform mat4 modelMat;   // Model to World transform
uniform mat4 viewMat;    // World to View transform
uniform mat4 normalMat;
uniform float sphereBlend;

out vec3 fragNormal;
out vec2 fragTex;
out vec3 fragPos;					    // World space
out vec3 fragDirectionalLightSource;    // View space
out vec3 fragSpotLightDirection;	    // View space
out vec3 fragSpotLightRelativePosition; // View space

vec3 to_sphere(vec3 vertex) {
	return vertex;  // TODO: Convert to sphere
}

void main() {
	// Constants:
	vec3 directionalLightSource = vec3(3.0, 5.0, 1.0);
	vec3 spotLightSource = vec3(0.0, 5.0, 3.0);
	vec3 spotLightDirection = vec3(0, 1, -2);

	// Vertex:
	vec4 vertex = vec4(mix(vertVertex, to_sphere(vertVertex), sphereBlend), 1.0);
	// TODO: Morph normals

	// Output:
	fragPos = vec3(modelMat * vertex); 	  // World position
	fragNormal = normalize(mat3(normalMat) * vertNormal); // World normals
	fragTex = vertTex;									  // Texture coordinates

	// Directional light:
	fragDirectionalLightSource = normalize(mat3(viewMat) * directionalLightSource);

	// Spot light
	fragSpotLightDirection = normalize(mat3(viewMat) * (spotLightDirection - spotLightSource));
	fragSpotLightRelativePosition = mat3(viewMat) * vertex.xyz - mat3(viewMat) * spotLightSource;

	// Postion output
	gl_Position = mvp * vertex;
}

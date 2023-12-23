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
out vec3 fragSpotLightDirection;	    // View space
out vec3 fragSpotLightRelativePosition; // View space

float sphere_sqrt_component(vec2 sq) {
	float core = 1 - sq.x / 2 - sq.y / 2 + sq.x * sq.y / 3;
	return sqrt(core);
}

vec3 to_sphere(vec3 vertex) {
	vec3 squared = vertex * vertex;

	vec3 sqrt = vec3(
		sphere_sqrt_component(squared.yz),
		sphere_sqrt_component(squared.xz),
		sphere_sqrt_component(squared.xy)
	);

	return vertex * sqrt;
}

void main() {
	// Constants:
	vec3 spotLightSource = vec3(0.0, 5.0, 3.0);			// Position in World space
	vec3 spotLightDirection = vec3(0, 1, -2);			// Direction

	// Vertex:
	vec4 vertex = vec4(mix(vertVertex, to_sphere(vertVertex), 0), 1.0);
	vec3 normal = mix(vertNormal, normalize(vertVertex), 0);

	// Output:
	fragPos = vec3(modelMat * vertex); 	 				  // World position
	fragNormal = normalize(mat3(normalMat) * normal); // World normals
	fragTex = vertTex;									  // Texture coordinates

	// Spot light
	fragSpotLightDirection = normalize(mat3(viewMat) * (spotLightDirection - spotLightSource));
	fragSpotLightRelativePosition = mat3(viewMat) * vertex.xyz - mat3(viewMat) * spotLightSource;

	// Postion output
	gl_Position = mvp * vertex;
}

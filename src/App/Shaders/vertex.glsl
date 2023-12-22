#version 330 core

layout(location=0) in vec3 vertVertex;
layout(location=1) in vec3 vertNormal;
layout(location=2) in vec2 vertTex;

uniform mat4 mvp;
uniform mat4 modelMat;   // Model to World transform
uniform mat4 viewMat;    // World to View transform
uniform mat4 normalMat;

out vec3 fragNormal;
out vec2 fragTex;
out vec3 fragPos;
out vec3 fragDirectionalLightSource;

void main() {
	// Constants:
	vec3 lightDir = vec3(1.0, 1.0, 1.0);
	vec3 directionalLightSource = vec3(3.0, 5.0, 1.0);

	// Output:
	fragNormal = normalize(mat3(normalMat) * vertNormal);
	fragPos = vec3(modelMat * vec4(vertVertex, 1.0f));
	fragDirectionalLightSource = normalize(mat3(viewMat) * directionalLightSource);
	fragTex = vertTex;
	gl_Position = mvp * vec4(vertVertex, 1.0);
}

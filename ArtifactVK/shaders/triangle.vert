#version 450

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec3 outColor;

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
	//float test;
} Params;

void main() {
	gl_Position = Params.projection * Params.model * Params.view * vec4(vertexPosition, 0.0, 1.0);
	//gl_Position = vec4(vertexPosition * Params.test, 0.0, 1.0);
	outColor = vertexColor;
}

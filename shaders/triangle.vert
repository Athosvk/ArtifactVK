#version 450

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUv;

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} Params;

void main() {
	gl_Position = Params.projection * Params.view * Params.model * vec4(vertexPosition, 1.0);
	outColor = vertexColor;
	outUv = uv;
}

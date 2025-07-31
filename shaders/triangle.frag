#version 450

layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

void main() {
	outColor = texture(textureSampler, uv * 2);
	//outColor = vec4(uv, 0.0, 1.0);
}

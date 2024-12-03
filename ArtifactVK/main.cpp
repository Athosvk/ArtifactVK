// ArtifactVK.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main()
{
	// TODO: Move to app init?
	glfwInit();
	App app;
	app.RunRenderLoop();
	return 0;
}


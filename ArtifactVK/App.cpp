#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

App::App() : 
	m_Window(WindowCreateInfo{ 800, 600, "ArtifactVK" }),
	m_VulkanInstance("ArtifactVK")
{
}

App::~App()
{
	glfwTerminate();
}

void App::RunRenderLoop()
{
	while (!m_Window.ShouldClose()) 
	{
		m_Window.PollEvents();
	}
}

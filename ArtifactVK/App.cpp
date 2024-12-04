#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

const InstanceCreateInfo DefaultCreateInfo()
{
	InstanceCreateInfo createInfo;
	createInfo.Name = "ArtifactVK";
	createInfo.ValidationLayers = std::vector<ValidationLayer>{ ValidationLayer { EValidationLayer::KhronosValidation, false } };
	return createInfo;
}

App::App() : 
	m_VulkanInstance(DefaultCreateInfo()),
	m_Window(WindowCreateInfo{ 800, 600, "ArtifactVK" })
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

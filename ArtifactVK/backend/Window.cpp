#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

Window::Window(const WindowCreateInfo& windowParams)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_InternalWindow = glfwCreateWindow(windowParams.Width, windowParams.Height, windowParams.Name.c_str(), nullptr, nullptr);
}

Window::~Window()
{
	glfwDestroyWindow(m_InternalWindow);
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_InternalWindow);
}

void Window::PollEvents() const
{
	glfwPollEvents();
}

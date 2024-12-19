#include "VulkanSurface.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

VulkanSurface::VulkanSurface(const VkInstance& instance, GLFWwindow& internalWindow) :
	m_Surface(CreateSurface(instance, internalWindow)),
	m_VkInstance(instance)
{
}

VulkanSurface::~VulkanSurface()
{
	vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
}

bool VulkanSurface::IsSupportedOnQueue(const VkPhysicalDevice& device, uint32_t queueIndex) const
{
	VkBool32 isSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, queueIndex, m_Surface, &isSupported);
	return isSupported == VK_TRUE;
}

VkSurfaceKHR VulkanSurface::CreateSurface(const VkInstance& instance, GLFWwindow& internalWindow)
{
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance, &internalWindow, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create surface for rendering");
	}
	return surface;
}

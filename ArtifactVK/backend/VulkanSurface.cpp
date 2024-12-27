#include "VulkanSurface.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

VulkanSurface::VulkanSurface(const VkInstance &instance, GLFWwindow &internalWindow)
    : m_Surface(CreateSurface(instance, internalWindow)), m_VkInstance(instance)
{
}

VulkanSurface::VulkanSurface(VulkanSurface &&other)
    : m_Surface(std::exchange(other.m_Surface, VK_NULL_HANDLE)), m_VkInstance(other.m_VkInstance)
{
}

VulkanSurface::~VulkanSurface()
{
    if (m_Surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
    }
}

bool VulkanSurface::IsSupportedOnQueue(const VkPhysicalDevice &device, uint32_t queueIndex) const
{
    VkBool32 isSupported;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, queueIndex, m_Surface, &isSupported);
    return isSupported == VK_TRUE;
}

SurfaceProperties VulkanSurface::QueryProperties(const VkPhysicalDevice &device) const
{
    SurfaceProperties surfaceProperties;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &surfaceProperties.Capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

    if (formatCount > 0)
    {
        surfaceProperties.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, surfaceProperties.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
        surfaceProperties.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount,
                                                  surfaceProperties.PresentModes.data());
    }
    return surfaceProperties;
}

VkSurfaceKHR VulkanSurface::CreateSurface(const VkInstance &instance, GLFWwindow &internalWindow)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, &internalWindow, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create surface for rendering");
    }
    return surface;
}

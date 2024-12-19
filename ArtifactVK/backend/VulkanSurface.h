#pragma once
#include <vulkan/vulkan.h>

struct GLFWwindow;

class VulkanSurface
{
public:
	VulkanSurface(const VkInstance& instance, GLFWwindow& internalWindow);
	VulkanSurface(const VulkanSurface& other) = delete;
	VulkanSurface(VulkanSurface&& other) = delete;
	~VulkanSurface();

	bool IsSupportedOnQueue(const VkPhysicalDevice& device, uint32_t queueIndex) const;
private:
	static VkSurfaceKHR CreateSurface(const VkInstance& instance, GLFWwindow& internalWindow);

	VkSurfaceKHR m_Surface;
	const VkInstance& m_VkInstance;
};

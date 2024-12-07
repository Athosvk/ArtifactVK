#pragma once
#include <vulkan/vulkan.h>

class VulkanExtensionMapper;

class VulkanDebugMessenger
{
public:
	VulkanDebugMessenger(VkInstance& vulkanInstance, const VulkanExtensionMapper& extensionMapper);
	~VulkanDebugMessenger();
private:
	static VkDebugUtilsMessengerEXT Create(VkInstance& vulkanInstance, const VulkanExtensionMapper& extensionMapper);

	VkInstance& m_VulkanInstance;
	const VulkanExtensionMapper& m_ExtensionMapper;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
};


#pragma once
#include <vulkan/vulkan.h>

class ExtensionFunctionMapping;

class VulkanDebugMessenger
{
public:
	VulkanDebugMessenger(VkInstance& vulkanInstance, const ExtensionFunctionMapping& extensionMapper);
	~VulkanDebugMessenger();
	VulkanDebugMessenger(const VulkanDebugMessenger& other) = delete;
	VulkanDebugMessenger(VulkanDebugMessenger&& other);

	static VkDebugUtilsMessengerCreateInfoEXT CreateInfo();
private:
	static VkDebugUtilsMessengerEXT Create(VkInstance& vulkanInstance, const ExtensionFunctionMapping& extensionMapper);

	VkInstance& m_VulkanInstance;
	const ExtensionFunctionMapping& m_ExtensionMapper;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
};


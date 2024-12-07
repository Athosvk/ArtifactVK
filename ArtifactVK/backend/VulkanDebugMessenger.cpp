#include "VulkanDebugMessenger.h"

#include <vulkan/vulkan.h>
#include <iostream>

#include "VulkanExtensionMapper.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	std::string severity;
	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 
		severity = "TRACE";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		severity = "INFO";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		severity = "WARNING";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		severity = "ERROR";
		break;
	}

	std::cout << severity << ": Validation layer: " << callbackData->pMessage << std::endl;
	return VK_FALSE;
}

VulkanDebugMessenger::VulkanDebugMessenger(VkInstance& vulkanInstance, const VulkanExtensionMapper& extensionMapper) :
	m_VulkanInstance(vulkanInstance),
	m_ExtensionMapper(extensionMapper),
	m_DebugMessenger(Create(m_VulkanInstance, m_ExtensionMapper))
{
}

VulkanDebugMessenger::~VulkanDebugMessenger()
{
	auto destroyDebugManager = (PFN_vkDestroyDebugUtilsMessengerEXT)m_ExtensionMapper.GetFunction(EExtensionFunction::VkDestroyDebugUtilsMessengerEXT);
	destroyDebugManager(m_VulkanInstance, m_DebugMessenger, nullptr);
}

VkDebugUtilsMessengerEXT VulkanDebugMessenger::Create(VkInstance& vulkanInstance, const VulkanExtensionMapper& extensionMapper)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;

	auto createFunction = (PFN_vkCreateDebugUtilsMessengerEXT)extensionMapper.GetFunction(EExtensionFunction::VkCreateDebugUtilsMessengerExt);
	VkDebugUtilsMessengerEXT debugMessenger;
	// TODO: Handle errors here
	createFunction(vulkanInstance, &createInfo, nullptr, &debugMessenger);
	return debugMessenger;
}

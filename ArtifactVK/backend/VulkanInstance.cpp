#include "VulkanInstance.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>

const uint32_t Version::ToVulkanVersion() const
{
	return VK_MAKE_API_VERSION(0, Major, Minor, Patch);
}

VulkanInstance::VulkanInstance(const InstanceCreateInfo& createInfo)
{
	VkApplicationInfo appInfo {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = createInfo.Name.c_str();
	appInfo.applicationVersion = createInfo.AppVersion.ToVulkanVersion();
	appInfo.pEngineName = "Artifact";
	appInfo.engineVersion = createInfo.EngineVersion.ToVulkanVersion();
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = nullptr;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	instanceInfo.enabledExtensionCount = glfwExtensionCount;
	instanceInfo.ppEnabledExtensionNames = glfwExtensions;
	instanceInfo.enabledLayerCount = 0;


	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &m_VKInstance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create vulkan instance");
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	for (const auto& extension : extensions)
	{
		std::cout << "\t" << extension.extensionName << "\n";
	}


	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, glfwExtensions[i]) != 0)
			{
				hasExtension = true;
			}
		}
		if (!hasExtension)
		{
			std::cout << "Missing required glfw extension: " << glfwExtensions[i] << "\n";
			throw std::runtime_error("Missing extension for GLFW");
		}
	}
	std::cout << "Found all extensions to init GLFW";
}

VulkanInstance::~VulkanInstance()
{
	vkDestroyInstance(m_VKInstance, nullptr);
}


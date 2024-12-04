#include "VulkanInstance.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <immintrin.h>

#ifndef NDEBUG
#define NDEBUG 0
#endif // !NDEBUG

std::vector<const char*> ValidationLayer::GetLayerNames() const
{
	std::vector<const char*> names;
	names.reserve(_mm_popcnt_u32((uint32_t)Layers));
	for (EValidationLayer availableLayer : AvailableValidationLayers())
	{
		if (((uint32_t)availableLayer & (uint32_t)Layers) == (uint32_t)availableLayer)
		{
			switch (availableLayer)
			{
			case EValidationLayer::KhronosValidation:
				names.emplace_back("VK_LAYER_KHRONOS_validation\0");
			}
		}
	}
	return names;
}

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
			if (strcmp(extension.extensionName, glfwExtensions[i]) == 0)
			{
				hasExtension = true;
			}
		}
		if (!hasExtension)
		{
			// TODO: Have GLFW request its extensions instead
			std::cout << "Missing required glfw extension: " << glfwExtensions[i] << "\n";
			throw std::runtime_error("Missing extension for GLFW");
		}
	}
	std::cout << "Found all extensions to init GLFW\n";

	auto requestedValidationLayers = CheckValidationLayers(createInfo.ValidationLayers);
	if (!requestedValidationLayers.empty())
	{
		instanceInfo.enabledLayerCount = (uint32_t)requestedValidationLayers.size();
		instanceInfo.ppEnabledLayerNames = requestedValidationLayers.data();
		std::cout << "Enabled " << requestedValidationLayers.size() << " validation layers";
	}
	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &m_VKInstance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create vulkan instance");
	}
}

VulkanInstance::~VulkanInstance()
{
	vkDestroyInstance(m_VKInstance, nullptr);
}

std::vector<const char*> VulkanInstance::CheckValidationLayers(const std::vector<ValidationLayer>& validationLayers) const
{
	std::vector<const char*> requestedLayers;
	for (const auto& validationLayer : validationLayers)
	{
		if (!validationLayer.DebugOnly || !NDEBUG)
		{
			const std::vector<const char*> layerNames = validationLayer.GetLayerNames();
			requestedLayers.reserve(requestedLayers.size() + layerNames.size());
			requestedLayers.insert(requestedLayers.end(), layerNames.begin(), layerNames.end());
		}
	}

	if (requestedLayers.empty())
	{
		return requestedLayers;
	}

	uint32_t availableLayerCount;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

	std::vector<VkLayerProperties> availableLayerProperties(availableLayerCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayerProperties.data());

	for (const auto& requestedLayerName : requestedLayers)
	{
		bool layerFound = false;
		for (const auto& availableLayer : availableLayerProperties)
		{
			if (strcmp(requestedLayerName, availableLayer.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			std::cout << "Missing layer " << requestedLayerName;
			return {};
		}
	}
	return requestedLayers;
}

const std::array<EValidationLayer, 1> AvailableValidationLayers()
{
	return { EValidationLayer::KhronosValidation };
}

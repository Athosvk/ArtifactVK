#include "VulkanInstance.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <immintrin.h>
#include <unordered_map>
#include <string_view>

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
	: m_VkInstance(Create(createInfo)),
	m_ExtensionMapper(m_VkInstance)
{
	m_VulkanDebugMessenger.ScopeBegin(m_VkInstance, m_ExtensionMapper);
}

VulkanInstance::~VulkanInstance()
{
	m_VulkanDebugMessenger.ScopeEnd();
	vkDestroyInstance(m_VkInstance, nullptr);
}


std::vector<const char*> VulkanInstance::CheckValidationLayers(const std::vector<ValidationLayer>& validationLayers)
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

	std::vector<VkLayerProperties> availableLayers(availableLayerCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

	std::unordered_map<std::string_view, bool>  foundRequestedDebugLayers;

	for (const auto& requestedLayerName : requestedLayers)
	{
		foundRequestedDebugLayers[std::string_view{ requestedLayerName }] = false;
	}

	for (const auto& availableLayer : availableLayers)
	{
		foundRequestedDebugLayers[std::string_view{ availableLayer.layerName }] = true;
	}

	std::vector<const char*> layers;
	layers.reserve(foundRequestedDebugLayers.size());
	for (const auto& [layerName, wasFound] : foundRequestedDebugLayers)
	{
		if (!wasFound)
		{
			std::cout << "Missing layer " << layerName;
		}
		else
		{
			layers.emplace_back(layerName.data());
		}
	}
	return requestedLayers;
}

VkInstance VulkanInstance::Create(const InstanceCreateInfo& createInfo)
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

	auto requestedValidationLayers = CheckValidationLayers(createInfo.ValidationLayers);

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> requestedExtensions(glfwExtensionCount);
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		requestedExtensions[i] = glfwExtensions[i];
	}
	if (!requestedValidationLayers.empty())
	{
		requestedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	uint32_t availableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

	std::cout << "Available extensions: ";
	for (const auto& extension : availableExtensions)
	{
		std::cout << "\t" << extension.extensionName << "\n";
	}

	std::unordered_map<std::string_view, bool> foundRequestedExtensions;
	for (const auto& extension : requestedExtensions)
	{
		foundRequestedExtensions[std::string_view { extension }] = false;
	}

	for (const auto& extension : availableExtensions)
	{
		foundRequestedExtensions[std::string_view{ extension.extensionName }] = true;
	}

	bool anyNotFound = false;
	for (const auto& [name, wasFound] : foundRequestedExtensions)
	{
		if (!wasFound)
		{
			anyNotFound = true;
			// TODO: Have GLFW request its extensions instead
			std::cout << "Missing requested extension: " << name << "\n";
		}
	}
	if (anyNotFound)
	{
		throw std::runtime_error("Missing extension");
	}
	std::cout << "Found all extensions to init Vulkan\n";

	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size());
	instanceInfo.ppEnabledExtensionNames = requestedExtensions.data();
	instanceInfo.enabledLayerCount = 0;
	if (!requestedValidationLayers.empty())
	{
		instanceInfo.enabledLayerCount = (uint32_t)requestedValidationLayers.size();
		instanceInfo.ppEnabledLayerNames = requestedValidationLayers.data();
		std::cout << "Enabled " << requestedValidationLayers.size() << " validation layers\n";
	}
	auto debugMessengerCreateInfo = VulkanDebugMessenger::CreateInfo();
	// Create a temporary debug messenger for logging Vulkan instance creation issues
	instanceInfo.pNext = &debugMessengerCreateInfo;
	VkInstance vkInstance = nullptr;
	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vkInstance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create vulkan instance");
	}

	return vkInstance;
}

const std::array<EValidationLayer, 1> AvailableValidationLayers()
{
	return { EValidationLayer::KhronosValidation };
}

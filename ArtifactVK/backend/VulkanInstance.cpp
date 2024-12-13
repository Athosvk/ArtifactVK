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
	: m_VkInstance(CreateInstance(createInfo)),
	m_ExtensionMapper(VulkanExtensionMapper(m_VkInstance)),
	m_ActiveDevice(CreatePhysicalDevice())
{
	// TODO: Move to initializer list so that debug messenges are not delayed
	m_VulkanDebugMessenger.ScopeBegin(m_VkInstance, m_ExtensionMapper);
	m_ActiveLogicalDevice.ScopeBegin(m_ActiveDevice, m_ValidationLayers);
}

VulkanInstance::~VulkanInstance()
{
	m_VulkanDebugMessenger.ScopeEnd();
	m_ActiveLogicalDevice.ScopeEnd();
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

VkInstance VulkanInstance::CreateInstance(const InstanceCreateInfo& createInfo)
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

	m_ValidationLayers = CheckValidationLayers(createInfo.ValidationLayers);

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> requestedExtensions(glfwExtensionCount);
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		requestedExtensions[i] = glfwExtensions[i];
	}
	if (!m_ValidationLayers.empty())
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
	if (!m_ValidationLayers.empty())
	{
		instanceInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
		instanceInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		std::cout << "Enabled " << m_ValidationLayers.size() << " validation layers\n";
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

VulkanDevice VulkanInstance::CreatePhysicalDevice() const
{
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_VkInstance, &count, nullptr);

	if (count == 0)
	{
		throw std::runtime_error("No available VK devices found");
	}

	std::vector<VkPhysicalDevice> physicalDevices(count);

	vkEnumeratePhysicalDevices(m_VkInstance, &count, physicalDevices.data());

	std::vector<VulkanDevice> devices;
	devices.reserve(physicalDevices.size());
	for (auto& physicalDevice : physicalDevices)
	{
		// TODO: Early exit if we find the first suitable one?
		devices.emplace_back(VulkanDevice(
			std::move(physicalDevice)));
	}

	// Score device or get preference from somwhere
	auto firstValid = std::find_if(devices.begin(), devices.end(), [this](const VulkanDevice& device)
		{
			return device.IsValid();
		});
	if (firstValid == devices.end())
	{
		throw std::runtime_error("No suitable VK devices found");
	}
	return *firstValid;
}

bool VulkanDevice::Validate() const
{
	return (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
		m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
		m_Features.geometryShader &&
	m_QueueFamilies.GraphicsFamily.has_value();
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies() const
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());
	
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[static_cast<size_t>(i)].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 1)
		{
			indices.GraphicsFamily = i;
			break;
		}
	}
	return indices;
}

VkDevice VulkanInstance::CreateLogicalDevice(const VulkanDevice& physicalDevice) const 
{
	VkDeviceQueueCreateInfo queueCreateInfo{};
	assert(physicalDevice.IsValid() && "Not a valid device to create a logical device from");
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = physicalDevice.GetQueueFamilies().GraphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	// Also specify here for backwards compatability with old vulkan implementations.
	// This shouldn't functionally change the enabled validation layers
	if (!m_ValidationLayers.empty())
	{
		deviceCreateInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();
	} 
	else 
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}
	deviceCreateInfo.pEnabledFeatures = &physicalDevice.GetFeatures();
	
	VkDevice device;
	if (vkCreateDevice(physicalDevice.GetInternal(), &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create logical device");
	}
	return device;
}


const std::array<EValidationLayer, 1> AvailableValidationLayers()
{
	return { EValidationLayer::KhronosValidation };
}

VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice) :
	m_PhysicalDevice(physicalDevice),
	m_QueueFamilies(FindQueueFamilies()),
	m_Properties(QueryDeviceProperties()),
	m_Features(QueryDeviceFeatures()),
	m_Valid(Validate())
{
}

bool VulkanDevice::IsValid() const
{
	return m_Valid;
}


const QueueFamilyIndices& VulkanDevice::GetQueueFamilies() const
{
	return m_QueueFamilies;
}

const VkPhysicalDeviceProperties& VulkanDevice::GetProperties() const
{
	return m_Properties;
}

const VkPhysicalDeviceFeatures& VulkanDevice::GetFeatures() const
{
	return m_Features;
}

const VkPhysicalDevice& VulkanDevice::GetInternal() const
{
	return m_PhysicalDevice;
}

VkPhysicalDeviceProperties VulkanDevice::QueryDeviceProperties() const
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
	return properties;
}

VkPhysicalDeviceFeatures VulkanDevice::QueryDeviceFeatures() const
{
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &features);
	return features;
}

LogicalVulkanDevice::LogicalVulkanDevice(const VulkanDevice& physicalDevice, const std::vector<const char*>& validationLayers)
{
	VkDeviceQueueCreateInfo queueCreateInfo{};
	assert(physicalDevice.IsValid() && "Not a valid device to create a logical device from");
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = physicalDevice.GetQueueFamilies().GraphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	// Also specify here for backwards compatability with old vulkan implementations.
	// This shouldn't functionally change the enabled validation layers
	if (!validationLayers.empty())
	{
		deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	} 
	else 
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}
	deviceCreateInfo.pEnabledFeatures = &physicalDevice.GetFeatures();
	
	if (vkCreateDevice(physicalDevice.GetInternal(), &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create logical device");
	}
}

LogicalVulkanDevice::~LogicalVulkanDevice()
{
	vkDestroyDevice(m_Device, nullptr);
}

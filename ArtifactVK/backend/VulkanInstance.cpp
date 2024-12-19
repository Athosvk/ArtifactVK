#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "VulkanInstance.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <immintrin.h>
#include <unordered_map>
#include <string_view>
#include <thread>
#include <condition_variable>
#include <set>

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

VulkanInstance::VulkanInstance(const InstanceCreateInfo& createInfo, GLFWwindow& window) : 
	m_VkInstance(CreateInstance(createInfo)),
	m_ExtensionMapper(ExtensionFunctionMapping(m_VkInstance))
{
	m_VulkanDebugMessenger.ScopeBegin(m_VkInstance, m_ExtensionMapper);

	m_Surface.ScopeBegin(m_VkInstance, window);
	m_ActiveDevice.ScopeBegin(CreatePhysicalDevice(*m_Surface, std::span { createInfo.RequiredExtensions }));
	m_ActiveLogicalDevice.ScopeBegin(*m_ActiveDevice, m_ValidationLayers);
}

VulkanInstance::~VulkanInstance()
{
	m_VulkanDebugMessenger.ScopeEnd();
	m_ActiveLogicalDevice.ScopeEnd();
	m_ActiveDevice.ScopeEnd();
	m_Surface.ScopeEnd();
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

VulkanDevice VulkanInstance::CreatePhysicalDevice(const VulkanSurface& targetSurface, std::span<const EDeviceExtension> requestedExtensions) const
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
		// TODO: Early exit if we find the first suitable one, depending on
		// preference function.
		devices.emplace_back(std::move(physicalDevice), targetSurface, m_DeviceExtensionMapper, requestedExtensions);
	}

	auto firstValid = devices.end();
	uint32_t numValidDevices = 0;
	for (auto iter = devices.begin(); iter != devices.end(); iter++)
	{
		if (iter->IsValid())
		{
			firstValid = iter;
			numValidDevices++;
		}
	}
	if (firstValid == devices.end())
	{
		throw std::runtime_error("No suitable VK devices found");
	}
	std::cout << "Found " << numValidDevices << " suitable physical devices\n";
	return std::move(*firstValid);
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

LogicalVulkanDevice::LogicalVulkanDevice(const VulkanDevice& physicalDevice, const std::vector<const char*>& validationLayers)
{
	assert(physicalDevice.IsValid() && "Need a valid physical device");

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = GetQueueCreateInfos(physicalDevice);

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
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
	// Assertion: physical device has a graphics family queue
	vkGetDeviceQueue(m_Device, physicalDevice.GetQueueFamilies().GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, physicalDevice.GetQueueFamilies().PresentFamily.value(), 0, &m_PresentQueue);
}

LogicalVulkanDevice::~LogicalVulkanDevice()
{
	std::condition_variable destroyed;
	std::mutex destroyMutex;
	std::thread destroyThread([this, &destroyed, &destroyMutex] {
		std::unique_lock lock(destroyMutex);
		vkDeviceWaitIdle(m_Device);
		destroyed.notify_one();
	});
	std::unique_lock lock(destroyMutex);
	if (destroyed.wait_for(lock, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
		std::cout << "ERROR: Waited for > 500 ms for queue operations to finish. Forcibly deleting device";
		// Detach, not going to wait for a blocking call. We already announced we're forcefully
		// deleting the device here.
		destroyThread.detach();
	}
	else {
		destroyThread.join();
	}
	vkDestroyDevice(m_Device, nullptr);
}

std::vector<VkDeviceQueueCreateInfo> LogicalVulkanDevice::GetQueueCreateInfos(const VulkanDevice& physicalDevice)
{
	std::set<uint32_t> uniqueQueueIndices = physicalDevice.GetQueueFamilies().GetUniqueQueues();

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(uniqueQueueIndices.size());
	for (uint32_t queueIndex : uniqueQueueIndices) {
		VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
		graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueCreateInfo.queueFamilyIndex = queueIndex;
		graphicsQueueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		graphicsQueueCreateInfo.pQueuePriorities = &priority;
		queueCreateInfos.emplace_back(graphicsQueueCreateInfo);
	}
	return queueCreateInfos;
}

std::set<uint32_t> QueueFamilyIndices::GetUniqueQueues() const
{
	return { GraphicsFamily.value(), PresentFamily.value() };
}

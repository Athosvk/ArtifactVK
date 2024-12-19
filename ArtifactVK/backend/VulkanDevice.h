#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <set>
#include <span>

#include "DeviceExtensionMapping.h"

class VulkanSurface;

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;

	std::set<uint32_t> GetUniqueQueues() const;
};

class VulkanDevice;

class LogicalVulkanDevice
{
public: 
	LogicalVulkanDevice(const VulkanDevice& physicalDevice, const VkPhysicalDevice& physicalDeviceHandle, 
		const std::vector<const char*>& validationLayers, std::vector<EDeviceExtension> extensions,
		const DeviceExtensionMapping& deviceExtensionMapping
		);
	LogicalVulkanDevice(const LogicalVulkanDevice& other) = delete;
	LogicalVulkanDevice(LogicalVulkanDevice&& other) = default;
	~LogicalVulkanDevice();
private:
	static std::vector<VkDeviceQueueCreateInfo> GetQueueCreateInfos(const VulkanDevice& physicalDevice);

	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
	std::vector<EDeviceExtension> m_Extensions;
};


class VulkanDevice
{
public:
	VulkanDevice(VkPhysicalDevice physicalDevice, const VulkanSurface& targetSurface, const DeviceExtensionMapping& extensionMapping, 
		std::span<const EDeviceExtension> requestedExtensions);
	VulkanDevice(const VulkanDevice& other) = delete;
	VulkanDevice(VulkanDevice&& other) = default;

	const QueueFamilyIndices& GetQueueFamilies() const;
	bool IsValid() const;
	const VkPhysicalDeviceProperties& GetProperties() const;
	const VkPhysicalDeviceFeatures& GetFeatures() const;
	std::vector<EDeviceExtension> FilterAvailableExtensions(std::span<const EDeviceExtension> desiredExtensions) const;
	LogicalVulkanDevice CreateLogicalDevice(const std::vector<const char*>& validationLayers, std::vector<EDeviceExtension> extensions);
private:
	bool Validate(std::span<const EDeviceExtension> requiredExtensions) const;
	bool AllExtensionsAvailable(std::span<const EDeviceExtension> extensions) const;
	std::set<EDeviceExtension> QueryExtensions(const DeviceExtensionMapping& extensionMapping) const;
	QueueFamilyIndices FindQueueFamilies(const VulkanSurface& surface) const;
	VkPhysicalDeviceProperties QueryDeviceProperties() const;
	VkPhysicalDeviceFeatures QueryDeviceFeatures() const;

	const DeviceExtensionMapping& m_ExtensionMapping;
	VkPhysicalDevice m_PhysicalDevice;
	QueueFamilyIndices m_QueueFamilies;
	VkPhysicalDeviceProperties m_Properties;
	VkPhysicalDeviceFeatures m_Features;
	std::set<EDeviceExtension> m_AvailableExtensions;
	bool m_Valid;
};


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

class VulkanDevice
{
public:
	VulkanDevice(VkPhysicalDevice physicalDevice, const VulkanSurface& targetSurface, const DeviceExtensionMapping& extensionMapping);
	VulkanDevice(const VulkanDevice& other) = delete;
	VulkanDevice(VulkanDevice&& other) = default;

	const QueueFamilyIndices& GetQueueFamilies() const;
	bool IsValid() const;
	const VkPhysicalDeviceProperties& GetProperties() const;
	const VkPhysicalDeviceFeatures& GetFeatures() const;
	const VkPhysicalDevice& GetInternal() const;
private:
	bool Validate(std::span<EDeviceExtension> requestedExtensions) const;
	std::set<EDeviceExtension> QueryExtensions(const DeviceExtensionMapping& extensionMapping) const;
	QueueFamilyIndices FindQueueFamilies(const VulkanSurface& surface) const;
	VkPhysicalDeviceProperties QueryDeviceProperties() const;
	VkPhysicalDeviceFeatures QueryDeviceFeatures() const;

	VkPhysicalDevice m_PhysicalDevice;
	QueueFamilyIndices m_QueueFamilies;
	VkPhysicalDeviceProperties m_Properties;
	VkPhysicalDeviceFeatures m_Features;
	std::set<EDeviceExtension> m_AvailableExtensions;
	bool m_Valid;
};


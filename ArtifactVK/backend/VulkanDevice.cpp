#include "VulkanDevice.h"

#include "VulkanSurface.h"

VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice, const VulkanSurface& targetSurface, const DeviceExtensionMapping& extensionMapping, 
	std::span<const EDeviceExtension> requestedExtensions) :
	m_PhysicalDevice(physicalDevice),
	m_QueueFamilies(FindQueueFamilies(targetSurface)),
	m_Properties(QueryDeviceProperties()),
	m_Features(QueryDeviceFeatures()),
	m_AvailableExtensions(QueryExtensions(extensionMapping)),
	m_Valid(Validate(requestedExtensions))
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

bool VulkanDevice::Validate(std::span<const EDeviceExtension> requiredExtensions) const
{
	return (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
		m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
		m_Features.geometryShader &&
		m_QueueFamilies.GraphicsFamily.has_value() &&
		m_QueueFamilies.PresentFamily.has_value() &&
		AllExtensionsAvailable(requiredExtensions);
}

bool VulkanDevice::AllExtensionsAvailable(std::span<const EDeviceExtension> extensions) const
{
	bool allPresent = true;
	for (auto extension : extensions)
	{
		allPresent = allPresent && m_AvailableExtensions.find(extension) != m_AvailableExtensions.end();
	}
	return allPresent;
}

std::set<EDeviceExtension> VulkanDevice::QueryExtensions(const DeviceExtensionMapping& extensionMapping) const
{
	uint32_t extensionCount;
	// TODO: Embed support for layer-based extensions
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, extensions.data());
	
	std::set<EDeviceExtension> mappedExtensions;
	for (const auto& extension : extensions)
	{
		mappedExtensions.insert(extensionMapping.At(extension.extensionName));
	}
	return mappedExtensions;
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(const VulkanSurface& surface) const
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
		} 
		if (!indices.PresentFamily.has_value() && surface.IsSupportedOnQueue(m_PhysicalDevice, i))
		{
			indices.PresentFamily = i;
		}
	}

	return indices;
}


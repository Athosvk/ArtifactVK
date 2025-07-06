#include "PhysicalDevice.h"

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice,
                           std::optional<std::reference_wrapper<const VulkanSurface>> targetSurface,
                           const DeviceExtensionMapping &extensionMapping,
                           std::span<const EDeviceExtension> requestedExtensions)
    : m_ExtensionMapping(extensionMapping), m_PhysicalDevice(physicalDevice),
      m_QueueFamilies(FindQueueFamilies(targetSurface)), m_Properties(QueryDeviceProperties()),
      m_Features(QueryDeviceFeatures()), 
      m_MemoryProperties(QueryMemoryProperties()),
      m_SurfaceProperties(QuerySurfaceProperties(targetSurface)),
      m_AvailableExtensions(QueryExtensions(extensionMapping)), m_Valid(Validate(requestedExtensions)),
      m_TargetSurface(targetSurface)
{
}

bool PhysicalDevice::IsValid() const
{
    return m_Valid;
}

const QueueFamilyIndices &PhysicalDevice::GetQueueFamilies() const
{
    return m_QueueFamilies;
}

const VkPhysicalDeviceProperties &PhysicalDevice::GetProperties() const
{
    return m_Properties;
}

const VkPhysicalDeviceFeatures &PhysicalDevice::GetFeatures() const
{
    return m_Features;
}

std::vector<EDeviceExtension> PhysicalDevice::FilterAvailableExtensions(
    std::span<const EDeviceExtension> desiredExtensions) const
{
    std::vector<EDeviceExtension> desiredAvailableExtensions;
    desiredAvailableExtensions.reserve(desiredExtensions.size());
    for (EDeviceExtension extension : desiredExtensions)
    {
        desiredAvailableExtensions.emplace_back(extension);
    }
    return desiredAvailableExtensions;
}

VulkanDevice PhysicalDevice::CreateLogicalDevice(const std::vector<const char *> &validationLayers,
                                                      std::vector<EDeviceExtension> extensions, GLFWwindow& window)
{
    return VulkanDevice(*this, m_PhysicalDevice, validationLayers, extensions, m_ExtensionMapping, window);
}

SurfaceProperties PhysicalDevice::GetCachedSurfaceProperties() const
{
    return m_SurfaceProperties;
}

SurfaceProperties PhysicalDevice::QuerySurfaceProperties()
{
    m_SurfaceProperties = QuerySurfaceProperties(m_TargetSurface);
    return m_SurfaceProperties;
}

VkPhysicalDeviceMemoryProperties PhysicalDevice::MemoryProperties() const
{
    return m_MemoryProperties;
}

VkPhysicalDeviceProperties PhysicalDevice::QueryDeviceProperties() const
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    return properties;
}

VkPhysicalDeviceFeatures PhysicalDevice::QueryDeviceFeatures() const
{
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &features);
    return features;
}

SurfaceProperties PhysicalDevice::QuerySurfaceProperties(
    std::optional<std::reference_wrapper<const VulkanSurface>> surface) const
{

    if (surface)
    {
        return surface->get().QueryProperties(m_PhysicalDevice);
    }
    else
    {
        return SurfaceProperties{};
    }
}

VkPhysicalDeviceMemoryProperties PhysicalDevice::QueryMemoryProperties() const
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
    return memoryProperties;
}

bool PhysicalDevice::Validate(std::span<const EDeviceExtension> requiredExtensions) const
{
    return (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
           m_Features.geometryShader && m_QueueFamilies.GraphicsFamilyIndex.has_value() &&
           m_QueueFamilies.PresentFamilyIndex.has_value() && !m_SurfaceProperties.Formats.empty() &&
           !m_SurfaceProperties.PresentModes.empty() && AllExtensionsAvailable(requiredExtensions) &&
           m_Features.samplerAnisotropy;
}

bool PhysicalDevice::AllExtensionsAvailable(std::span<const EDeviceExtension> extensions) const
{
    bool allPresent = true;
    for (auto extension : extensions)
    {
        allPresent = allPresent && m_AvailableExtensions.find(extension) != m_AvailableExtensions.end();
    }
    return allPresent;
}

std::set<EDeviceExtension> PhysicalDevice::QueryExtensions(const DeviceExtensionMapping &extensionMapping) const
{
    uint32_t extensionCount;
    // TODO: Embed support for layer-based extensions
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, extensions.data());

    std::set<EDeviceExtension> mappedExtensions;
    for (const auto &extension : extensions)
    {
        mappedExtensions.insert(extensionMapping.At(extension.extensionName));
    }
    return mappedExtensions;
}

QueueFamilyIndices PhysicalDevice::FindQueueFamilies(
    std::optional<std::reference_wrapper<const VulkanSurface>> surface) const
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[static_cast<size_t>(i)].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamilyIndex = i;
        }

        if (queueFamilies[static_cast<size_t>(i)].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.ComputeFamilyIndex = i;
        }
        
        if (!indices.PresentFamilyIndex.has_value() && surface && (*surface).get().IsSupportedOnQueue(m_PhysicalDevice, i))
        {
            indices.PresentFamilyIndex = i;
        }

        if (queueFamilies[static_cast<size_t>(i)].queueFlags & VK_QUEUE_TRANSFER_BIT
            // Explicitly look for a dedicated transfer queue. We'll use the compute or graphics
            // queue if there is none
            && !(queueFamilies[static_cast<size_t>(i)].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            && !(queueFamilies[static_cast<size_t>(i)].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            indices.TransferFamilyIndex = i;
        }
    }

    if (!indices.TransferFamilyIndex)
    {
        // We can always assume either the graphics or compute queue allows for transfer operations
        if (indices.GraphicsFamilyIndex)
        {
            indices.TransferFamilyIndex = indices.GraphicsFamilyIndex;
        }
        else if (indices.ComputeFamilyIndex)
        {
            indices.TransferFamilyIndex = indices.ComputeFamilyIndex;
        }
    }

    return indices;
}


uint32_t PhysicalDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) const
{
    for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) != 0
			&& (m_MemoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
		{
			return i;
		}
	}
	throw std::runtime_error("Could not find suitable memory type for type filter: " + std::to_string(typeFilter));
}

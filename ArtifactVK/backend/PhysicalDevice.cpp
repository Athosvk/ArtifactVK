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

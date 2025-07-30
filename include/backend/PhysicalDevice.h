#pragma once
#include <vulkan/vulkan.h>

#include <optional>
#include <set>

#include "DeviceExtensionMapping.h"
#include "VulkanSurface.h"
#include "VulkanDevice.h"

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamilyIndex;
    std::optional<uint32_t> PresentFamilyIndex;
    std::optional<uint32_t> TransferFamilyIndex;
    std::optional<uint32_t> ComputeFamilyIndex;

    std::set<uint32_t> GetUniqueQueues() const;
};

class PhysicalDevice
{
  public:
    PhysicalDevice(VkPhysicalDevice physicalDevice,
                 std::optional<std::reference_wrapper<const VulkanSurface>> targetSurface,
                 const DeviceExtensionMapping &extensionMapping, std::span<const EDeviceExtension> requestedExtensions);
    PhysicalDevice(const PhysicalDevice &other) = delete;
    PhysicalDevice(PhysicalDevice&& other) = default;

    const QueueFamilyIndices& GetQueueFamilies() const;
    bool IsValid() const;
    const VkPhysicalDeviceProperties& GetProperties() const;
    const VkPhysicalDeviceFeatures& GetFeatures() const;
    std::vector<EDeviceExtension> FilterAvailableExtensions(std::span<const EDeviceExtension> desiredExtensions) const;
    VulkanDevice CreateLogicalDevice(const std::vector<const char *> &validationLayers,
                                                 std::vector<EDeviceExtension> extensions, GLFWwindow &window,
                                                 const VulkanInstance &instance);

    // TODO: These are the cached values, but not neccessarily the latest. Need to requery this possibly
    SurfaceProperties GetCachedSurfaceProperties() const;
    SurfaceProperties QuerySurfaceProperties();
    VkPhysicalDeviceMemoryProperties MemoryProperties() const;

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) const;
    VkFormat FindFirstSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling,
                                VkFormatFeatureFlags features) const;
  private:
    VkPhysicalDeviceMemoryProperties QueryMemoryProperties() const;
    bool Validate(std::span<const EDeviceExtension> requiredExtensions) const;
    bool AllExtensionsAvailable(std::span<const EDeviceExtension> extensions) const;
    std::set<EDeviceExtension> QueryExtensions(const DeviceExtensionMapping &extensionMapping) const;
    QueueFamilyIndices FindQueueFamilies(std::optional<std::reference_wrapper<const VulkanSurface>> surface) const;
    VkPhysicalDeviceProperties QueryDeviceProperties() const;
    VkPhysicalDeviceFeatures QueryDeviceFeatures() const;
    SurfaceProperties QuerySurfaceProperties(std::optional<std::reference_wrapper<const VulkanSurface>> surface) const;

    VkPhysicalDevice m_PhysicalDevice;
    const DeviceExtensionMapping &m_ExtensionMapping;
    QueueFamilyIndices m_QueueFamilies;
    VkPhysicalDeviceProperties m_Properties;
    VkPhysicalDeviceFeatures m_Features;
    VkPhysicalDeviceMemoryProperties m_MemoryProperties;
    SurfaceProperties m_SurfaceProperties;
    std::set<EDeviceExtension> m_AvailableExtensions;
    std::optional<std::reference_wrapper<const VulkanSurface>> m_TargetSurface;
    bool m_Valid;
};

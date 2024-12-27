#include "VulkanDevice.h"

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "VulkanSurface.h"

VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice,
                           std::optional<std::reference_wrapper<const VulkanSurface>> targetSurface,
                           const DeviceExtensionMapping &extensionMapping,
                           std::span<const EDeviceExtension> requestedExtensions)
    : m_ExtensionMapping(extensionMapping), m_PhysicalDevice(physicalDevice),
      m_QueueFamilies(FindQueueFamilies(targetSurface)), m_Properties(QueryDeviceProperties()),
      m_Features(QueryDeviceFeatures()), m_SurfaceProperties(QuerySurfaceProperties(targetSurface)),
      m_AvailableExtensions(QueryExtensions(extensionMapping)), m_Valid(Validate(requestedExtensions))
{
}

bool VulkanDevice::IsValid() const
{
    return m_Valid;
}

const QueueFamilyIndices &VulkanDevice::GetQueueFamilies() const
{
    return m_QueueFamilies;
}

const VkPhysicalDeviceProperties &VulkanDevice::GetProperties() const
{
    return m_Properties;
}

const VkPhysicalDeviceFeatures &VulkanDevice::GetFeatures() const
{
    return m_Features;
}

std::vector<EDeviceExtension> VulkanDevice::FilterAvailableExtensions(
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

LogicalVulkanDevice VulkanDevice::CreateLogicalDevice(const std::vector<const char *> &validationLayers,
                                                      std::vector<EDeviceExtension> extensions)
{
    return LogicalVulkanDevice(*this, m_PhysicalDevice, validationLayers, extensions, m_ExtensionMapping);
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

SurfaceProperties VulkanDevice::QuerySurfaceProperties(
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

bool VulkanDevice::Validate(std::span<const EDeviceExtension> requiredExtensions) const
{
    return (m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            m_Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
           m_Features.geometryShader && m_QueueFamilies.GraphicsFamily.has_value() &&
           m_QueueFamilies.PresentFamily.has_value() && !m_SurfaceProperties.Formats.empty() &&
           !m_SurfaceProperties.PresentModes.empty() && AllExtensionsAvailable(requiredExtensions);
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

std::set<EDeviceExtension> VulkanDevice::QueryExtensions(const DeviceExtensionMapping &extensionMapping) const
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

QueueFamilyIndices VulkanDevice::FindQueueFamilies(
    std::optional<std::reference_wrapper<const VulkanSurface>> surface) const
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
        if (!indices.PresentFamily.has_value() && surface && (*surface).get().IsSupportedOnQueue(m_PhysicalDevice, i))
        {
            indices.PresentFamily = i;
        }
    }

    return indices;
}

LogicalVulkanDevice::LogicalVulkanDevice(const VulkanDevice &device, const VkPhysicalDevice &physicalDeviceHandle,
                                         const std::vector<const char *> &validationLayers,
                                         std::vector<EDeviceExtension> extensions,
                                         const DeviceExtensionMapping &deviceExtensionMapping)
    : m_Extensions(std::move(extensions))
{
    assert(device.IsValid() && "Need a valid physical device");

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = GetQueueCreateInfos(device);

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
    auto extensionNames = deviceExtensionMapping.ReverseMap(std::span<EDeviceExtension>{extensions});
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    deviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

    deviceCreateInfo.pEnabledFeatures = &device.GetFeatures();

    if (vkCreateDevice(physicalDeviceHandle, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create logical device");
    }
    // Assertion: physical device has a graphics family queue
    vkGetDeviceQueue(m_Device, device.GetQueueFamilies().GraphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, device.GetQueueFamilies().PresentFamily.value(), 0, &m_PresentQueue);
}

LogicalVulkanDevice::LogicalVulkanDevice(LogicalVulkanDevice &&other)
    : m_Device(std::exchange(other.m_Device, VK_NULL_HANDLE)), m_GraphicsQueue(other.m_GraphicsQueue),
      m_PresentQueue(other.m_PresentQueue), m_Extensions(other.m_Extensions)
{
}

LogicalVulkanDevice::~LogicalVulkanDevice()
{
    if (m_Device == VK_NULL_HANDLE)
    {
        return;
    }
    std::condition_variable destroyed;
    std::mutex destroyMutex;
    std::thread destroyThread([this, &destroyed, &destroyMutex] {
        std::unique_lock lock(destroyMutex);
        vkDeviceWaitIdle(m_Device);
        destroyed.notify_one();
    });
    std::unique_lock lock(destroyMutex);
    if (destroyed.wait_for(lock, std::chrono::milliseconds(500)) == std::cv_status::timeout)
    {
        std::cout << "ERROR: Waited for > 500 ms for queue operations to finish. Forcibly deleting device";
        // Detach, not going to wait for a blocking call. We already announced we're forcefully
        // deleting the device here.
        destroyThread.detach();
    }
    else
    {
        destroyThread.join();
    }
    vkDestroyDevice(m_Device, nullptr);
}

std::vector<VkDeviceQueueCreateInfo> LogicalVulkanDevice::GetQueueCreateInfos(const VulkanDevice &physicalDevice)
{
    std::set<uint32_t> uniqueQueueIndices = physicalDevice.GetQueueFamilies().GetUniqueQueues();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueIndices.size());
    for (uint32_t queueIndex : uniqueQueueIndices)
    {
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
    return {GraphicsFamily.value(), PresentFamily.value()};
}

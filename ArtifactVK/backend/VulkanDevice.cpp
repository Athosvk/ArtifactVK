#include "VulkanDevice.h"

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <utility>
#include <array>

#include <GLFW/glfw3.h>

#include "VulkanSurface.h"
#include "Window.h"
#include "ShaderModule.h"

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
                                                      std::vector<EDeviceExtension> extensions, GLFWwindow& window)
{
    return LogicalVulkanDevice(*this, m_PhysicalDevice, validationLayers, extensions, m_ExtensionMapping, window);
}

const SurfaceProperties& VulkanDevice::GetSurfaceProperties() const
{
    return m_SurfaceProperties;
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

VkSurfaceFormatKHR LogicalVulkanDevice::SelectSurfaceFormat() const
{
    auto surfaceProperties = m_PhysicalDevice.GetSurfaceProperties();
    auto iter = std::find_if(surfaceProperties.Formats.begin(), surfaceProperties.Formats.end(), [](const VkSurfaceFormatKHR& format)
        {
        return format.colorSpace == VkColorSpaceKHR::VK_COLORSPACE_SRGB_NONLINEAR_KHR && format.format ==
            VkFormat::VK_FORMAT_B8G8R8A8_SRGB; 
        });
    if (iter != surfaceProperties.Formats.end())
    {
        return *iter;
    } 
    else
    {
        // Prefer the one above, but return something in case that fails.
        // TODO: Better surface format selection
        return surfaceProperties.Formats.front();
    }
}

VkPresentModeKHR LogicalVulkanDevice::SelectPresentMode() const
{
    auto surfaceProperties = m_PhysicalDevice.GetSurfaceProperties();
    return std::find(surfaceProperties.PresentModes.begin(), surfaceProperties.PresentModes.end(), VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR) != surfaceProperties.PresentModes.end() ?
        VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR : VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D LogicalVulkanDevice::SelectSwapchainExtent(GLFWwindow& window) const
{
    auto surfaceProperties = m_PhysicalDevice.GetSurfaceProperties();
    if (surfaceProperties.Capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max())
    {
        return surfaceProperties.Capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(&window, &width, &height);
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };
        
        actualExtent.width = std::clamp(actualExtent.width, surfaceProperties.Capabilities.minImageExtent.width, surfaceProperties.Capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceProperties.Capabilities.minImageExtent.height, surfaceProperties.Capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void LogicalVulkanDevice::CreateSwapchain(GLFWwindow& window, const VulkanSurface& surface)
{
    auto maxImageCount = m_PhysicalDevice.GetSurfaceProperties().Capabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max()
                             : m_PhysicalDevice.GetSurfaceProperties().Capabilities.maxImageCount;
    SwapchainCreateInfo createInfo{
        SelectSurfaceFormat(),
        SelectPresentMode(),
        SelectSwapchainExtent(window),
        // Select min image count + 1 if available
        std::min(m_PhysicalDevice.GetSurfaceProperties().Capabilities.minImageCount + 1,
            maxImageCount)
    };
       
    m_Swapchain.emplace(Swapchain(createInfo, surface.Get(), m_Device, m_PhysicalDevice));
}

ShaderModule LogicalVulkanDevice::LoadShaderModule(const std::filesystem::path &filename)
{
    return ShaderModule::LoadFromDisk(m_Device, filename);
}

RasterPipeline LogicalVulkanDevice::CreateRasterPipeline(RasterPipelineBuilder &&pipelineBuilder)
{
    auto fragmentShader = LoadShaderModule(pipelineBuilder.GetFragmentShaderPath());
    auto vertexShader = LoadShaderModule(pipelineBuilder.GetVertexShaderPath());

    VkPipelineShaderStageCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragCreateInfo.module = fragmentShader.Get();
    fragCreateInfo.pName = "main";
    fragCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo vertexCreateInfo{};
    vertexCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexCreateInfo.module = vertexShader.Get();
    vertexCreateInfo.pName = "main";
    vertexCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo stages[] = {fragCreateInfo, vertexCreateInfo};

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_VIEWPORT,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
    return RasterPipeline {};
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

LogicalVulkanDevice::LogicalVulkanDevice(const VulkanDevice &physicalDevice, const VkPhysicalDevice &physicalDeviceHandle,
                        const std::vector<const char *> &validationLayers, std::vector<EDeviceExtension> extensions,
                        const DeviceExtensionMapping &deviceExtensionMapping, GLFWwindow& window)
    : m_PhysicalDevice(physicalDevice)
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
    auto extensionNames = deviceExtensionMapping.ReverseMap(std::span<EDeviceExtension>{extensions});
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    deviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

    deviceCreateInfo.pEnabledFeatures = &physicalDevice.GetFeatures();

    if (vkCreateDevice(physicalDeviceHandle, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create logical device");
    }
    // Assertion: physical device has a graphics family queue
    vkGetDeviceQueue(m_Device, physicalDevice.GetQueueFamilies().GraphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, physicalDevice.GetQueueFamilies().PresentFamily.value(), 0, &m_PresentQueue);
}

LogicalVulkanDevice::LogicalVulkanDevice(LogicalVulkanDevice &&other)
    : m_Device(std::exchange(other.m_Device, VK_NULL_HANDLE)), m_PhysicalDevice(other.m_PhysicalDevice),
      m_GraphicsQueue(other.m_GraphicsQueue), m_PresentQueue(other.m_PresentQueue),
      m_Swapchain(std::move(other.m_Swapchain))
{
}

LogicalVulkanDevice::~LogicalVulkanDevice()
{
    if (m_Device == VK_NULL_HANDLE)
    {
        return;
    }
    // Order matters, swapchain has to be destroyed first
    m_Swapchain.reset();
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

void LogicalVulkanDevice::CreatePipelineStage()
{
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

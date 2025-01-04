#include "Swapchain.h"

#include <stdexcept>
#include <algorithm>

#include "VulkanSurface.h"
#include "VulkanDevice.h"

Swapchain::Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, const VkDevice& device, const VulkanDevice& vulkanDevice) : m_VkDevice(device)
{
    VkSwapchainCreateInfoKHR vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkCreateInfo.surface = surface;
    vkCreateInfo.minImageCount = createInfo.MinImageCount;
    vkCreateInfo.imageFormat = createInfo.SurfaceFormat.format;
    vkCreateInfo.imageColorSpace = createInfo.SurfaceFormat.colorSpace;
    vkCreateInfo.imageExtent = createInfo.Extents;
    vkCreateInfo.imageArrayLayers = 1;
    vkCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto queueFamilies = vulkanDevice.GetQueueFamilies();
    if (queueFamilies.GraphicsFamily != queueFamilies.PresentFamily) {
        uint32_t indices[] = {queueFamilies.GraphicsFamily.value(), queueFamilies.PresentFamily.value()};
        vkCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   
        vkCreateInfo.queueFamilyIndexCount = 2;
        vkCreateInfo.pQueueFamilyIndices = indices;
    }
    else
    {
        vkCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // Just filler, no need to provide these in exclusive mode
        vkCreateInfo.queueFamilyIndexCount = 0;
        vkCreateInfo.pQueueFamilyIndices = nullptr;
    }

    SurfaceProperties surfaceProperties = vulkanDevice.GetSurfaceProperties();
    vkCreateInfo.preTransform = surfaceProperties.Capabilities.currentTransform;

    vkCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;   
    vkCreateInfo.presentMode = createInfo.PresentMode;
    vkCreateInfo.clipped = VK_TRUE;
    vkCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &vkCreateInfo, nullptr, &m_Swapchain) != VK_SUCCESS) 
    {
        throw std::runtime_error("Could not create swapchain");
    }
}

Swapchain::Swapchain(Swapchain &&other) :
    m_Swapchain(std::exchange(other.m_Swapchain, VK_NULL_HANDLE)), m_VkDevice(other.m_VkDevice)
{
}

Swapchain::~Swapchain()
{
    if (m_Swapchain != VK_NULL_HANDLE) 
    {
        vkDestroySwapchainKHR(m_VkDevice, m_Swapchain, nullptr);
    }
}

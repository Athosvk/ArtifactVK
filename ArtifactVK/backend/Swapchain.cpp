#include "Swapchain.h"

#include <stdexcept>
#include <algorithm>

#include "VulkanSurface.h"
#include "VulkanDevice.h"

Swapchain::Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, const VkDevice& device, const VulkanDevice& vulkanDevice) : 
    m_VkDevice(device), m_OriginalCreateInfo(createInfo)
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

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_VkDevice, m_Swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_VkDevice, m_Swapchain, &imageCount, images.data());
    m_Images = std::move(images);

    m_ImageViews.reserve(images.size());
    for (auto& image : m_Images)
    {
        VkImageViewCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageCreateInfo.image = image;
        imageCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        imageCreateInfo.format = createInfo.SurfaceFormat.format;
        imageCreateInfo.components = {
            VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,
            VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY };
        imageCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        imageCreateInfo.subresourceRange.baseMipLevel = 0;
        imageCreateInfo.subresourceRange.levelCount = 1;
        imageCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageCreateInfo.subresourceRange.layerCount = 1;
        
        VkImageView imageView;
        if (vkCreateImageView(m_VkDevice, &imageCreateInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create image view for swapchain image");
        }
        m_ImageViews.emplace_back(std::move(imageView));
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
        for (auto imageView : m_ImageViews) 
        {
            vkDestroyImageView(m_VkDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_VkDevice, m_Swapchain, nullptr);
    }
}

#include "Swapchain.h"

#include <stdexcept>
#include <algorithm>

#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "Semaphore.h"

Swapchain::Swapchain(const SwapchainCreateInfo &createInfo, const VkSurfaceKHR &surface, VkDevice device,
                     const VulkanDevice &vulkanDevice)
    : m_Device(device), m_OriginalCreateInfo(createInfo)
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
    if (queueFamilies.GraphicsFamilyIndex != queueFamilies.PresentFamilyIndex)
    {
        uint32_t indices[] = {queueFamilies.GraphicsFamilyIndex.value(), queueFamilies.PresentFamilyIndex.value()};
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
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, images.data());
    m_Images = std::move(images);

    m_ImageViews.reserve(images.size());
    for (auto &image : m_Images)
    {
        VkImageViewCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageCreateInfo.image = image;
        imageCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        imageCreateInfo.format = createInfo.SurfaceFormat.format;
        imageCreateInfo.components = {
            VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,
            VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY, VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY};
        imageCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        imageCreateInfo.subresourceRange.baseMipLevel = 0;
        imageCreateInfo.subresourceRange.levelCount = 1;
        imageCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageCreateInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(m_Device, &imageCreateInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create image view for swapchain image");
        }
        m_ImageViews.emplace_back(std::move(imageView));
    }
}

Swapchain::Swapchain(Swapchain &&other)
    : m_Swapchain(std::exchange(other.m_Swapchain, VK_NULL_HANDLE)), m_Device(other.m_Device),
      m_Images(std::move(other.m_Images)), m_ImageViews(std::move(other.m_ImageViews))
{
}

Swapchain::~Swapchain()
{
    if (m_Swapchain != VK_NULL_HANDLE) 
    {
        for (auto imageView : m_ImageViews) 
        {
            vkDestroyImageView(m_Device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    }
}

Viewport Swapchain::GetViewportDescription() const
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.height = static_cast<float>(m_OriginalCreateInfo.Extents.height);
    viewport.width = static_cast<float>(m_OriginalCreateInfo.Extents.width);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissorRect;
    scissorRect.extent = m_OriginalCreateInfo.Extents;
    scissorRect.offset = VkOffset2D { static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)};
    return {viewport, scissorRect};
}

VkAttachmentDescription Swapchain::AttachmentDescription() const
{
    VkAttachmentDescription attachmentDescription;

    attachmentDescription.format = m_OriginalCreateInfo.SurfaceFormat.format;
    attachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    return attachmentDescription;
}

std::vector<Framebuffer> Swapchain::CreateFramebuffersFor(const RenderPass& renderPass)
{
    std::vector<Framebuffer> framebuffers;
    framebuffers.reserve(m_ImageViews.size());
    for (const auto& imageView : m_ImageViews) 
    {
        framebuffers.emplace_back(
            Framebuffer(m_Device, FramebufferCreateInfo{renderPass, imageView, GetViewportDescription()}));
    }
    return framebuffers;
}

uint32_t Swapchain::Acquire(const Semaphore& semaphore)
{
    uint32_t imageIndex;
    if (vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), semaphore.Get(),
                              VK_NULL_HANDLE, &imageIndex) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Acquire next image failed");
    }
    return imageIndex;
}

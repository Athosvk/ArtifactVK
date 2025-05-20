#include "Swapchain.h"

#include <stdexcept>
#include <algorithm>
#include <iostream>

#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "Semaphore.h"

Swapchain::Swapchain(const SwapchainCreateInfo &createInfo, const VkSurfaceKHR &surface, VkDevice device,
                     const VulkanDevice &vulkanDevice,
                     Queue targetPresentQueue)
    : m_Device(device), 
    m_VulkanDevice(vulkanDevice),
    m_OriginalCreateInfo(createInfo), 
    m_TargetPresentQueue(targetPresentQueue)
{
    Create(createInfo, surface, device, vulkanDevice);
}

Swapchain::Swapchain(Swapchain &&other)
    : m_Swapchain(std::exchange(other.m_Swapchain, VK_NULL_HANDLE)), 
      m_Surface(std::exchange(other.m_Surface, VK_NULL_HANDLE)),
      m_Device(other.m_Device),
      m_VulkanDevice(other.m_VulkanDevice),
      m_OriginalCreateInfo(other.m_OriginalCreateInfo),
      m_Images(std::move(other.m_Images)),
      m_ImageViews(std::move(other.m_ImageViews)),
      m_CurrentImageIndex(std::move(other.m_CurrentImageIndex)),
      m_TargetPresentQueue(std::move(other.m_TargetPresentQueue))
{
}

Swapchain::~Swapchain()
{
    if (m_Swapchain != VK_NULL_HANDLE) 
    {
        Destroy();
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

SwapchainFramebuffer Swapchain::CreateFramebuffersFor(const RenderPass &renderPass) const
{
    std::vector<Framebuffer> framebuffers;
    framebuffers.reserve(m_ImageViews.size());
    for (const auto& imageView : m_ImageViews) 
    {
        framebuffers.emplace_back(
            Framebuffer(m_Device, FramebufferCreateInfo{renderPass, imageView, GetViewportDescription()}));
    }
    return SwapchainFramebuffer(*this, std::move(framebuffers), renderPass);
}

uint32_t Swapchain::CurrentIndex() const
{
    return m_CurrentImageIndex;
}

VkImageView Swapchain::AcquireNext(const Semaphore& toSignal)
{
    if (vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), toSignal.Get(),
                              VK_NULL_HANDLE, &m_CurrentImageIndex) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Acquire next image failed");
    }
    return m_ImageViews[m_CurrentImageIndex];
}

void Swapchain::Present(std::span<Semaphore> waitSempahores) const
{
    std::vector<VkSemaphore> semaphoreHandles;
    semaphoreHandles.reserve(waitSempahores.size());
    // TODO: Remove ugly allocation
    for (const auto &semaphore : waitSempahores)
    {
        semaphoreHandles.emplace_back(semaphore.Get());
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(semaphoreHandles.size());
    presentInfo.pWaitSemaphores = semaphoreHandles.data();

    VkSwapchainKHR swapchains[] = {m_Swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_CurrentImageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(m_TargetPresentQueue.Get(), &presentInfo);
    if (result == VkResult::VK_SUBOPTIMAL_KHR)
    {
        std::cout << "Recreate swapchain is optimal here";
    }
    else if (result != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not present to target queue");
    }
}

SwapchainFramebuffer Swapchain::Recreate(SwapchainFramebuffer &&oldFramebuffers)
{
    const RenderPass &renderPass = oldFramebuffers.GetRenderPass();
    // Explicitly destruct
    {
        SwapchainFramebuffer oldFramebuffers = std::move(oldFramebuffers);
    }
    Destroy();
    Create(m_OriginalCreateInfo, m_Surface, m_Device, m_VulkanDevice);
    return CreateFramebuffersFor(renderPass);
}

VkSwapchainKHR Swapchain::Create(const SwapchainCreateInfo &createInfo, const VkSurfaceKHR &surface, VkDevice device,
                                 const VulkanDevice &vulkanDevice)
{
    VkSwapchainCreateInfoKHR vkCreateInfo{};
    vkCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkCreateInfo.surface = surface;
    vkCreateInfo.minImageCount = createInfo.MinImageCount;
    vkCreateInfo.imageFormat = createInfo.SurfaceFormat.format;
    vkCreateInfo.imageColorSpace = createInfo.SurfaceFormat.colorSpace;
    vkCreateInfo.imageExtent = createInfo.Extents;
    vkCreateInfo.imageArrayLayers = 1;
    vkCreateInfo.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto queueFamilies = vulkanDevice.GetQueueFamilies();
    // TODO: Is this check correct?
    if (queueFamilies.GraphicsFamilyIndex != queueFamilies.PresentFamilyIndex)
    {
        uint32_t indices[] = {queueFamilies.GraphicsFamilyIndex.value(), queueFamilies.PresentFamilyIndex.value()};
        vkCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
        vkCreateInfo.queueFamilyIndexCount = 2;
        vkCreateInfo.pQueueFamilyIndices = indices;
    }
    else
    {
        vkCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
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

    if (vkCreateSwapchainKHR(device, &vkCreateInfo, nullptr, &m_Swapchain) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create swapchain");
    }

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, images.data());

    m_ImageViews.reserve(images.size());
    m_Images = std::move(images);
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

void Swapchain::Destroy()
{
	for (auto imageView : m_ImageViews) 
	{
		vkDestroyImageView(m_Device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

SwapchainFramebuffer::SwapchainFramebuffer(const Swapchain& swapchain, std::vector<Framebuffer>&& swapchainFramebuffers, const RenderPass& renderPass) : 
    m_Swapchain(swapchain), m_Framebuffers(std::move(swapchainFramebuffers)), m_Renderpass(renderPass)
{
}

const Framebuffer& SwapchainFramebuffer::GetCurrent() const
{
    return m_Framebuffers[m_Swapchain.CurrentIndex()];
}

const RenderPass &SwapchainFramebuffer::GetRenderPass() const
{
    return m_Renderpass;
}

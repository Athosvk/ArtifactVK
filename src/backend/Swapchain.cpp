#include <backend/Swapchain.h>

#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <cassert>

#include <backend/VulkanSurface.h>
#include <backend/VulkanDevice.h>
#include <backend/Semaphore.h>
#include <backend/PhysicalDevice.h>

const static uint32_t InvalidImageIndex = 0xFFFFFFFF;

Swapchain::Swapchain(const SwapchainCreateInfo &createInfo, const VkSurfaceKHR &surface, VkDevice device,
                     const PhysicalDevice &vulkanDevice,
                     Queue targetPresentQueue)
    : m_Device(device), 
    m_VulkanDevice(vulkanDevice),
    m_OriginalCreateInfo(createInfo), 
    m_TargetPresentQueue(targetPresentQueue), 
    m_State(SwapchainState::Optimal), m_Surface(surface)
{
    Create(createInfo, surface, device, vulkanDevice, VK_NULL_HANDLE);
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
      m_TargetPresentQueue(std::move(other.m_TargetPresentQueue)), 
      m_State(std::move(other.m_State))
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
    VkAttachmentDescription attachmentDescription{};

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

SwapchainFramebuffer Swapchain::CreateFramebuffersFor(const RenderPass &renderPass, DepthAttachment* depthAttachment) const
{
    std::vector<Framebuffer> framebuffers;
    framebuffers.reserve(m_ImageViews.size());
    for (const auto& imageView : m_ImageViews) 
    {
        framebuffers.emplace_back(
            Framebuffer(m_Device, FramebufferCreateInfo{renderPass, imageView, depthAttachment->GetView(), 
                GetViewportDescription()}));
    }
    return SwapchainFramebuffer(*this, std::move(framebuffers), renderPass, depthAttachment);
}

uint32_t Swapchain::CurrentIndex() const
{
    assert(m_State != SwapchainState::OutOfDate);
    assert(m_CurrentImageIndex != InvalidImageIndex && "Image index invalid, make sure you called AcquireNextImage");
    return m_CurrentImageIndex;
}

SwapchainState Swapchain::AcquireNext(const Semaphore& toSignal)
{
    assert(m_State != SwapchainState::OutOfDate);
    auto result = vkAcquireNextImageKHR(m_Device, m_Swapchain, std::numeric_limits<uint64_t>::max(), toSignal.Get(),
                                        VK_NULL_HANDLE, &m_CurrentImageIndex);
    m_State = MapResultToState(result);
    return m_State;
}

SwapchainState Swapchain::Present(std::span<Semaphore> waitSempahores)
{
    assert(m_CurrentImageIndex != InvalidImageIndex && "Image index invalid, make sure you called AcquireNextImage");
    assert(m_State != SwapchainState::OutOfDate);
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
    m_State = MapResultToState(result);
    // Reset it so that we can assert `AcquireNextImage` has always been called
    m_CurrentImageIndex = InvalidImageIndex;
    return m_State;
}

void Swapchain::Recreate(std::vector<std::unique_ptr<SwapchainFramebuffer>>& oldFramebuffers, VkExtent2D newExtents)
{
    // TODO: This functionality is probably overkill. Framework should/can assume
    // there's only a single render pass that ever renders to the final buffer.
    // All other render passes should blit to intermediate images.
    std::vector<const RenderPass *> renderPasses;
    // TODO: There should only be one depth attachment here
    std::vector<DepthAttachment*> depthAttachments;
    renderPasses.reserve(oldFramebuffers.size());
    depthAttachments.reserve(oldFramebuffers.size());
    for (const auto &framebuffer : oldFramebuffers)
    {
        renderPasses.emplace_back(&framebuffer->GetRenderPass());
        depthAttachments.emplace_back(framebuffer->GeDepthAttachment());
    }
    Destroy();
    
    // TODO: Maybe name this better or just implement this better altogether
    m_OriginalCreateInfo.Extents = newExtents;
    Create(m_OriginalCreateInfo, m_Surface, m_Device, m_VulkanDevice, VK_NULL_HANDLE);

    for (size_t i = 0; i < renderPasses.size(); i++) 
    {
        (*oldFramebuffers[i]) = CreateFramebuffersFor(*renderPasses[i], depthAttachments[i]);
    }
}

SwapchainState Swapchain::GetCurrentState() const
{
    return m_State;
}

void Swapchain::Create(const SwapchainCreateInfo &createInfo, const VkSurfaceKHR &surface, VkDevice device,
                                 const PhysicalDevice &vulkanDevice, VkSwapchainKHR oldSwapchain)
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

    SurfaceProperties surfaceProperties = vulkanDevice.GetCachedSurfaceProperties();
    vkCreateInfo.preTransform = surfaceProperties.Capabilities.currentTransform;

    vkCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkCreateInfo.presentMode = createInfo.PresentMode;
    vkCreateInfo.clipped = VK_TRUE;
    vkCreateInfo.oldSwapchain = oldSwapchain;

    std::cout << "Create swap size: " << createInfo.Extents.width << "," << createInfo.Extents.height
              << "\n";
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
    m_ImageViews.clear();
    m_Images.clear();
    
	vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    m_Swapchain = VK_NULL_HANDLE;
}

SwapchainState Swapchain::MapResultToState(VkResult result) const
{
    switch (result)
    {
    case VkResult::VK_SUBOPTIMAL_KHR:
        return SwapchainState::Suboptimal;    
    case VkResult::VK_ERROR_OUT_OF_DATE_KHR:
        return SwapchainState::OutOfDate;
    case VkResult::VK_SUCCESS:
        return SwapchainState::Optimal;
    default:
        throw std::runtime_error("Swapchain in invalid state or TDR");
    }
}

SwapchainFramebuffer::SwapchainFramebuffer(const Swapchain &swapchain, std::vector<Framebuffer> &&swapchainFramebuffers,
                                           const RenderPass &renderPass, DepthAttachment *depthAttachment)
    : m_Swapchain(swapchain), m_Framebuffers(std::move(swapchainFramebuffers)), m_Renderpass(renderPass),
      m_DepthAttachment(depthAttachment)
{
}

const Framebuffer &SwapchainFramebuffer::GetCurrent() const
{
    return m_Framebuffers[m_Swapchain.get().CurrentIndex()];
}

const RenderPass &SwapchainFramebuffer::GetRenderPass() const
{
    return m_Renderpass;
}

DepthAttachment *SwapchainFramebuffer::GeDepthAttachment() const
{
    return m_DepthAttachment;
}

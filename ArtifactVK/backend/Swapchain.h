#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <span>

#include "Framebuffer.h"

class VulkanDevice;
class RenderPass;
class Semaphore;
class Swapchain;

struct SwapchainCreateInfo
{
    VkSurfaceFormatKHR SurfaceFormat;
    VkPresentModeKHR PresentMode;
    VkExtent2D Extents;
    uint32_t MinImageCount;
};

class SwapchainFramebuffer
{
  public:
    SwapchainFramebuffer(const Swapchain& swapchain, std::vector<Framebuffer>&& m_SwapchainFramebuffers);
    SwapchainFramebuffer(const SwapchainFramebuffer&) = delete;
    SwapchainFramebuffer(SwapchainFramebuffer&&) = default;

    const Framebuffer& GetCurrent() const;
  private:
    // TODO: Make this a weak ptr for validation reasons?
    const Swapchain &m_Swapchain;
    std::vector<Framebuffer> m_Framebuffers;
};

class Swapchain
{
  public:
    Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, VkDevice device, const VulkanDevice& vulkanDevice, VkQueue targetPresentQueue);
    Swapchain(const Swapchain &other) = delete;
    Swapchain(Swapchain &&other);
    ~Swapchain();

    Viewport GetViewportDescription() const;
    VkAttachmentDescription AttachmentDescription() const;
    SwapchainFramebuffer CreateFramebuffersFor(const RenderPass &renderPass) const;
    uint32_t CurrentIndex() const;
    
    VkImageView AcquireNext(const Semaphore& toSignal);
    void Present(std::span<Semaphore> waitSempahores) const;
  private:
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkDevice m_Device;
    SwapchainCreateInfo m_OriginalCreateInfo;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    uint32_t m_CurrentImageIndex;
    VkQueue m_TargetPresentQueue;
};

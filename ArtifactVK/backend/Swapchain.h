#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <span>

#include "Framebuffer.h"
#include "Queue.h"

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
    SwapchainFramebuffer(const Swapchain& swapchain, std::vector<Framebuffer>&& m_SwapchainFramebuffers, const RenderPass& renderPass);
    SwapchainFramebuffer(const SwapchainFramebuffer&) = delete;
    SwapchainFramebuffer(SwapchainFramebuffer&&) = default;

    SwapchainFramebuffer &operator=(SwapchainFramebuffer &&other) = default;
    SwapchainFramebuffer &operator=(const SwapchainFramebuffer& other) = delete;

    const Framebuffer& GetCurrent() const;
    const RenderPass &GetRenderPass() const;
  private:
    // TODO: Make this a weak ptr for validation reasons?
    const Swapchain &m_Swapchain;
    std::vector<Framebuffer> m_Framebuffers;
    const RenderPass &m_Renderpass;
};

class Swapchain
{
  public:
    Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, VkDevice device, const VulkanDevice& vulkanDevice, Queue targetPresentQueue);
    Swapchain(const Swapchain &other) = delete;
    Swapchain(Swapchain &&other);
    ~Swapchain();

    Viewport GetViewportDescription() const;
    VkAttachmentDescription AttachmentDescription() const;
    SwapchainFramebuffer CreateFramebuffersFor(const RenderPass &renderPass) const;
    uint32_t CurrentIndex() const;
    
    VkImageView AcquireNext(const Semaphore& toSignal);
    void Present(std::span<Semaphore> waitSempahores) const;
    SwapchainFramebuffer Recreate(SwapchainFramebuffer&& oldFramebuffers, VkExtent2D newExtents);
  private:
    void Create(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, VkDevice device, const VulkanDevice& vulkanDevice);
    void Destroy();

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface;
    VkDevice m_Device;
    const VulkanDevice& m_VulkanDevice;
    SwapchainCreateInfo m_OriginalCreateInfo;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    uint32_t m_CurrentImageIndex = 0xFFFFFFFF;
    Queue m_TargetPresentQueue;
};

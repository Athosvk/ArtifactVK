#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <span>
#include <optional>
#include <memory>

#include "Framebuffer.h"
#include "Queue.h"

class PhysicalDevice;
class RenderPass;
class Semaphore;
class Swapchain;
class DepthAttachment;

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
    SwapchainFramebuffer(const Swapchain& swapchain, std::vector<Framebuffer>&& swapchainFramebuffers, 
        const RenderPass& renderPass, DepthAttachment* depthAttachment);
    SwapchainFramebuffer(const SwapchainFramebuffer&) = delete;
    SwapchainFramebuffer(SwapchainFramebuffer&&) = default;

    SwapchainFramebuffer &operator=(const SwapchainFramebuffer &&other) = delete;
    SwapchainFramebuffer &operator=(SwapchainFramebuffer &&other) = default;

    const Framebuffer &GetCurrent() const;
    const RenderPass &GetRenderPass() const;
    DepthAttachment *GeDepthAttachment() const;

  private:
    DepthAttachment* m_DepthAttachment;
    // TODO: Make this a weak ptr for validation reasons?
    std::reference_wrapper<const Swapchain> m_Swapchain;
    std::vector<Framebuffer> m_Framebuffers;
    std::reference_wrapper<const RenderPass> m_Renderpass;
};

enum class SwapchainState
{
	Suboptimal,
	OutOfDate,
	Optimal
};

class Swapchain
{
  public:
    Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, VkDevice device, const PhysicalDevice& vulkanDevice, Queue targetPresentQueue);
    Swapchain(const Swapchain &other) = delete;
    Swapchain(Swapchain &&other);
    ~Swapchain();

    Viewport GetViewportDescription() const;
    VkAttachmentDescription AttachmentDescription() const;
    SwapchainFramebuffer CreateFramebuffersFor(const RenderPass &renderPass, DepthAttachment* depthAttachment) const;
    uint32_t CurrentIndex() const;
    
    // Callers should check that the SwapchainState != SwapchainState::OutOfDate
    [[nodiscard]] 
        SwapchainState AcquireNext(const Semaphore& toSignal);
    // Callers should check that the SwapchainState != SwapchainState::OutOfDate
    [[nodiscard]] 
        SwapchainState Present(std::span<Semaphore> waitSemaphores);
    void Recreate(std::vector<std::unique_ptr<SwapchainFramebuffer>> &oldFramebuffers, VkExtent2D newExtents);
    SwapchainState GetCurrentState() const;
  private:
    void Create(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, VkDevice device, const PhysicalDevice& vulkanDevice, VkSwapchainKHR oldSwapchain);
    void Destroy();
    SwapchainState MapResultToState(VkResult result) const;

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface;
    VkDevice m_Device;
    const PhysicalDevice& m_VulkanDevice;
    SwapchainCreateInfo m_OriginalCreateInfo;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    uint32_t m_CurrentImageIndex = 0xFFFFFFFF;
    Queue m_TargetPresentQueue;
    SwapchainState m_State;
};

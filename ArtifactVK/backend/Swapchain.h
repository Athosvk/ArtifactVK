#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "Framebuffer.h"

class VulkanDevice;
class RenderPass;
class Semaphore;

struct SwapchainCreateInfo
{
    VkSurfaceFormatKHR SurfaceFormat;
    VkPresentModeKHR PresentMode;
    VkExtent2D Extents;
    uint32_t MinImageCount;
};

class Swapchain
{
  public:
    Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, VkDevice device, const VulkanDevice& vulkanDevice);
    Swapchain(const Swapchain &other) = delete;
    Swapchain(Swapchain &&other);
    ~Swapchain();

    Viewport GetViewportDescription() const;
    VkAttachmentDescription AttachmentDescription() const;
    std::vector<Framebuffer> CreateFramebuffersFor(const RenderPass& renderPass);
    uint32_t Acquire(const Semaphore& semaphore);
  private:
    VkSwapchainKHR m_Swapchain;
    VkDevice m_Device;
    SwapchainCreateInfo m_OriginalCreateInfo;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
};

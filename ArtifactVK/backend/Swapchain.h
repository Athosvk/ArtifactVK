#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

struct Viewport
{
    VkViewport Viewport;
    VkRect2D Scissor
};

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
    Swapchain(const SwapchainCreateInfo& createInfo, const VkSurfaceKHR& surface, const VkDevice& device, const VulkanDevice& vulkanDevice);
    Swapchain(const Swapchain &other) = delete;
    Swapchain(Swapchain &&other);
    ~Swapchain();

    Viewport GetViewportDescription() const;
  private:
    VkSwapchainKHR m_Swapchain;
    const VkDevice &m_VkDevice;
    SwapchainCreateInfo m_OriginalCreateInfo;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
};

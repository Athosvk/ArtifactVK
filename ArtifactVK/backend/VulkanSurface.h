#pragma once
#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

struct SurfaceProperties
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

class VulkanSurface
{
  public:
    VulkanSurface(const VkInstance &instance, GLFWwindow &internalWindow);
    VulkanSurface(const VulkanSurface &other) = delete;
    VulkanSurface(VulkanSurface &&other);
    ~VulkanSurface();

    bool IsSupportedOnQueue(const VkPhysicalDevice &device, uint32_t queueIndex) const;
    SurfaceProperties QueryProperties(const VkPhysicalDevice &device) const;
    const VkSurfaceKHR &Get() const;

  private:
    static VkSurfaceKHR CreateSurface(const VkInstance &instance, GLFWwindow &internalWindow);

    VkSurfaceKHR m_Surface;
    const VkInstance &m_VkInstance;
};

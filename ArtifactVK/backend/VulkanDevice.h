#pragma once
#include <functional>
#include <optional>
#include <set>
#include <span>
#include <vulkan/vulkan.h>
#include <optional>
#include <filesystem>
#include <span>

#include "DeviceExtensionMapping.h"
#include "VulkanSurface.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "CommandBufferPool.h"
#include "Fence.h"
#include "Semaphore.h"
#include "Queue.h"

class VulkanDevice;
struct GLFWwindow;
class ShaderModule;

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamilyIndex;
    std::optional<uint32_t> PresentFamilyIndex;

    std::set<uint32_t> GetUniqueQueues() const;
};

class LogicalVulkanDevice
{
  public:
    LogicalVulkanDevice(const VulkanDevice &physicalDevice, const VkPhysicalDevice &physicalDeviceHandle,
                        const std::vector<const char *> &validationLayers, std::vector<EDeviceExtension> extensions,
                        const DeviceExtensionMapping &deviceExtensionMapping, GLFWwindow& window);
    LogicalVulkanDevice(const LogicalVulkanDevice &other) = delete;
    LogicalVulkanDevice(LogicalVulkanDevice &&other);
    ~LogicalVulkanDevice();

    void WaitForIdle() const;
    Swapchain& CreateSwapchain(GLFWwindow& window, const VulkanSurface& surface);
    Swapchain &GetSwapchain();
    RasterPipeline CreateRasterPipeline(RasterPipelineBuilder &&pipelineBuilder, const RenderPass& renderPass);
    RenderPass CreateRenderPass();
    const SwapchainFramebuffer& CreateSwapchainFramebuffers(const RenderPass &renderpass);
    CommandBufferPool &CreateGraphicsCommandBufferPool();
    Semaphore &CreateDeviceSemaphore();
    Queue GetGraphicsQueue() const;
    void AcquireNext(const Semaphore& toSignal);
    void Present(std::span<Semaphore> waitSemaphores);
  private:
    void RecreateSwapchain();
    ShaderModule LoadShaderModule(const std::filesystem::path &filename);
    static std::vector<VkDeviceQueueCreateInfo> GetQueueCreateInfos(const VulkanDevice &physicalDevice);
    VkSurfaceFormatKHR SelectSurfaceFormat() const;
    VkPresentModeKHR SelectPresentMode() const;
    VkExtent2D SelectSwapchainExtent(GLFWwindow& window) const;

    VkDevice m_Device;
    GLFWwindow &m_Window;
    const VulkanDevice &m_PhysicalDevice;
    std::optional<Queue> m_GraphicsQueue;
    std::optional<Queue> m_PresentQueue;
    std::optional<Swapchain> m_Swapchain = std::nullopt;
    std::vector<std::unique_ptr<CommandBufferPool>> m_CommandBufferPools;
    // TODO: Don't hold the semaphores here (unless for pooling).
    // Let objects logically decide if they need to provide one.
    std::vector<std::unique_ptr<Semaphore>> m_Semaphores;
    // TODO: Manage this better using a delete queue/stack so that 
    // this doesn't have to manually manage these handles
    std::vector<std::unique_ptr<SwapchainFramebuffer>> m_SwapchainFramebuffers;
};

class VulkanDevice
{
  public:
    VulkanDevice(VkPhysicalDevice physicalDevice,
                 std::optional<std::reference_wrapper<const VulkanSurface>> targetSurface,
                 const DeviceExtensionMapping &extensionMapping, std::span<const EDeviceExtension> requestedExtensions);
    VulkanDevice(const VulkanDevice &other) = delete;
    VulkanDevice(VulkanDevice&& other) = default;

    const QueueFamilyIndices& GetQueueFamilies() const;
    bool IsValid() const;
    const VkPhysicalDeviceProperties& GetProperties() const;
    const VkPhysicalDeviceFeatures& GetFeatures() const;
    std::vector<EDeviceExtension> FilterAvailableExtensions(std::span<const EDeviceExtension> desiredExtensions) const;
    LogicalVulkanDevice CreateLogicalDevice(const std::vector<const char *>& validationLayers,
                                            std::vector<EDeviceExtension> extensions, GLFWwindow& window);

    const SurfaceProperties& GetSurfaceProperties() const;
  private:
    bool Validate(std::span<const EDeviceExtension> requiredExtensions) const;
    bool AllExtensionsAvailable(std::span<const EDeviceExtension> extensions) const;
    std::set<EDeviceExtension> QueryExtensions(const DeviceExtensionMapping &extensionMapping) const;
    QueueFamilyIndices FindQueueFamilies(std::optional<std::reference_wrapper<const VulkanSurface>> surface) const;
    VkPhysicalDeviceProperties QueryDeviceProperties() const;
    VkPhysicalDeviceFeatures QueryDeviceFeatures() const;
    SurfaceProperties QuerySurfaceProperties(std::optional<std::reference_wrapper<const VulkanSurface>> surface) const;

    VkPhysicalDevice m_PhysicalDevice;
    const DeviceExtensionMapping &m_ExtensionMapping;
    QueueFamilyIndices m_QueueFamilies;
    VkPhysicalDeviceProperties m_Properties;
    VkPhysicalDeviceFeatures m_Features;
    SurfaceProperties m_SurfaceProperties;
    std::set<EDeviceExtension> m_AvailableExtensions;
    bool m_Valid;
};

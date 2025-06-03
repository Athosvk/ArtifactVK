#pragma once
#include <functional>
#include <optional>
#include <set>
#include <span>
#include <vulkan/vulkan.h>
#include <optional>
#include <filesystem>
#include <span>
#include <typeinfo>

#include "DeviceExtensionMapping.h"
#include "VulkanSurface.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "CommandBufferPool.h"
#include "Fence.h"
#include "Semaphore.h"
#include "Queue.h"
#include "VertexBuffer.h"

class PhysicalDevice;
struct GLFWwindow;
class ShaderModule;
struct WindowResizeEvent;

class VulkanDevice
{
  public:
    VulkanDevice(PhysicalDevice &physicalDevice, const VkPhysicalDevice &physicalDeviceHandle,
                        const std::vector<const char *> &validationLayers, std::vector<EDeviceExtension> extensions,
                        const DeviceExtensionMapping &deviceExtensionMapping, GLFWwindow& window);
    VulkanDevice(const VulkanDevice &other) = delete;
    VulkanDevice(VulkanDevice &&other);
    ~VulkanDevice();

    void WaitForIdle() const;
    Swapchain& CreateSwapchain(GLFWwindow& window, const VulkanSurface& surface);
    Swapchain &GetSwapchain();
    RasterPipeline CreateRasterPipeline(RasterPipelineBuilder &&pipelineBuilder, const RenderPass& renderPass);
    RenderPass CreateRenderPass();
    const SwapchainFramebuffer& CreateSwapchainFramebuffers(const RenderPass &renderpass);
    // TODO: Make a getter, just construct it in the constructor 
    CommandBufferPool &CreateGraphicsCommandBufferPool();
    CommandBuffer &GetTransferCommandBuffer();
    Semaphore &CreateDeviceSemaphore();
    Queue GetGraphicsQueue() const;
    void AcquireNext(const Semaphore& toSignal);
    void Present(std::span<Semaphore> waitSemaphores);
    void HandleResizeEvent(const WindowResizeEvent &resizeEvent);
    template<typename T> 
    VertexBuffer &CreateVertexBuffer(std::vector<T> data)
    {
        auto bufferCreateInfo = CreateVertexBufferInfo{data};
        return m_VertexBuffers.emplace_back(bufferCreateInfo, m_Device, m_PhysicalDevice, GetTransferCommandBuffer());
    }
  private:
    CommandBufferPool CreateTransferCommandBufferPool() const;
    void RecreateSwapchain(VkExtent2D newSize);
    ShaderModule LoadShaderModule(const std::filesystem::path &filename);
    static std::vector<VkDeviceQueueCreateInfo> GetQueueCreateInfos(const PhysicalDevice &physicalDevice);
    VkSurfaceFormatKHR SelectSurfaceFormat() const;
    VkPresentModeKHR SelectPresentMode() const;
    VkExtent2D SelectSwapchainExtent(GLFWwindow& window, const SurfaceProperties& surfaceProperties) const;

    VkDevice m_Device;
    GLFWwindow &m_Window;
    PhysicalDevice &m_PhysicalDevice;
    std::optional<Queue> m_GraphicsQueue;
    std::optional<Queue> m_PresentQueue;
    std::optional<Swapchain> m_Swapchain = std::nullopt;
    std::vector<std::unique_ptr<CommandBufferPool>> m_CommandBufferPools;
    CommandBufferPool* m_TransferCommandBufferPool = nullptr;
    // TODO: Don't hold the semaphores here (unless for pooling).
    // Let objects logically decide if they need to provide one.
    std::vector<std::unique_ptr<Semaphore>> m_Semaphores;
    // TODO: Manage this better using a delete queue/stack so that 
    // this doesn't have to manually manage these handles
    std::vector<std::unique_ptr<SwapchainFramebuffer>> m_SwapchainFramebuffers;
    std::vector<VertexBuffer> m_VertexBuffers;
    std::optional<VkExtent2D> m_LastUnhandledResize;
};


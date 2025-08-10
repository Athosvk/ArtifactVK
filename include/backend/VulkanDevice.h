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
#include "ExtensionFunctionMapping.h"
#include "VulkanSurface.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "CommandBufferPool.h"
#include "Fence.h"
#include "Semaphore.h"
#include "Queue.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"
#include "DescriptorPool.h"
#include "Buffer.h"
#include "Texture.h"
#include "DescriptorSetBuilder.h"
#include "QueryPool.h"

class PhysicalDevice;
struct GLFWwindow;
class ShaderModule;
struct WindowResizeEvent;
class IndexBuffer;
class VulkanInstance;

class VulkanDevice
{
  public:
    VulkanDevice(PhysicalDevice &physicalDevice, VkPhysicalDevice physicalDeviceHandle,
                        const VulkanInstance& instance,
                        const std::vector<const char*> &validationLayers, std::vector<EDeviceExtension> extensions,
                        const DeviceExtensionMapping &deviceExtensionMapping, GLFWwindow& window);
    VulkanDevice(const VulkanDevice &other) = delete;
    VulkanDevice(VulkanDevice &&other);
    ~VulkanDevice();


    void WaitForIdle() const;
    Swapchain& CreateSwapchain(GLFWwindow& window, const VulkanSurface& surface);
    Swapchain &GetSwapchain();
    RasterPipeline CreateRasterPipeline(RasterPipelineBuilder &&pipelineBuilder, const RenderPass& renderPass);
    RenderPass CreateRenderPass(DepthAttachment& depthAttachment);
    const SwapchainFramebuffer& CreateSwapchainFramebuffers(const RenderPass &renderpass, DepthAttachment* depthAttachment);
    // TODO: Make a getter, just construct it in the constructor 
    CommandBufferPool CreateGraphicsCommandBufferPool();
    CommandBuffer &GetTransferCommandBuffer();
    CommandBufferPool &GetGraphicsCommandBufferPool();
    Semaphore &CreateDeviceSemaphore();
    Queue GetGraphicsQueue() const;
    Queue GetTransferQueue() const;
    void AcquireNext(const Semaphore& toSignal);
    void Present(std::span<Semaphore> waitSemaphores);
    void HandleResizeEvent(const WindowResizeEvent &resizeEvent);
    ExtensionFunctionMapping GetExtensionFunctionMapping() const;

    template<typename T> 
    VertexBuffer &CreateVertexBuffer(std::vector<T> initialData)
    {
        assert(m_GraphicsQueue.has_value() && "Need a graphics queue");
        auto bufferCreateInfo = CreateVertexBufferInfo{initialData, VkSharingMode::VK_SHARING_MODE_EXCLUSIVE, *m_GraphicsQueue };
        auto &commandBuffer = GetTransferCommandBuffer();
        commandBuffer.SetName("Vertex Buffer Transfer Command Buffer", GetExtensionFunctionMapping());
        return *m_VertexBuffers.emplace_back(std::make_unique<VertexBuffer>(bufferCreateInfo, m_Device, m_PhysicalDevice, commandBuffer));
    }

    IndexBuffer &CreateIndexBuffer(std::vector<uint16_t> data);

    template<typename T> 
    UniformBuffer &CreateUniformBuffer()
    {
        return *m_UniformBuffers.emplace_back(std::make_unique<UniformBuffer>(*this, m_Device, sizeof(T)));
    }

    DeviceBuffer &CreateBuffer(const CreateBufferInfo& createBufferInfo);
    Texture2D &CreateTexture(const Texture2DCreateInfo& createDesc);
    DepthAttachment &CreateSapchainDepthAttachment();
    // TODO: Store for re-use
    DescriptorSet CreateDescriptorSet(const DescriptorSetLayout& layout);
    const DescriptorSetLayout& CreateDescriptorSetLayout(DescriptorSetBuilder builder);
  private:
    CommandBufferPool CreateTransferCommandBufferPool() const;
    void RecreateSwapchain(VkExtent2D newSize);
    ShaderModule LoadShaderModule(const std::filesystem::path &filename);
    static std::vector<VkDeviceQueueCreateInfo> GetQueueCreateInfos(const PhysicalDevice &physicalDevice);
    VkSurfaceFormatKHR SelectSurfaceFormat() const;
    VkPresentModeKHR SelectPresentMode() const;
    VkExtent2D SelectSwapchainExtent(GLFWwindow& window, const SurfaceProperties& surfaceProperties) const;

    VkDevice m_Device;
    const VulkanInstance &m_Instance;
    GLFWwindow &m_Window;
    PhysicalDevice &m_PhysicalDevice;
    std::optional<Queue> m_GraphicsQueue;
    std::optional<Queue> m_PresentQueue;
    std::optional<Queue> m_TransferQueue;
    std::optional<Swapchain> m_Swapchain = std::nullopt;
    std::unique_ptr<CommandBufferPool> m_GraphicsCommandBufferPool;
    std::unique_ptr<CommandBufferPool> m_TransferCommandBufferPool = nullptr;
    std::optional<QueryPool> m_TimestampQueryPool;
    // TODO: Don't hold the semaphores here (unless for pooling).
    // Let objects logically decide if they need to provide one.
    std::vector<std::unique_ptr<Semaphore>> m_Semaphores;
    // TODO: Manage this better using a delete queue/stack so that 
    // this doesn't have to manually manage these handles
    std::vector<std::unique_ptr<SwapchainFramebuffer>> m_SwapchainFramebuffers;
    std::vector<std::unique_ptr<VertexBuffer>> m_VertexBuffers;
    std::vector<std::unique_ptr<IndexBuffer>> m_IndexBuffers;
    std::vector<std::unique_ptr<UniformBuffer>> m_UniformBuffers;
    std::vector<std::unique_ptr<Texture2D>> m_Textures;
    std::vector<std::unique_ptr<DepthAttachment>> m_DepthAttachments;
    std::vector<std::unique_ptr<DeviceBuffer>> m_Buffers; 
    std::optional<VkExtent2D> m_LastUnhandledResize;
    std::vector<std::unique_ptr<DescriptorSetLayout>> m_DescriptorSetLayouts;
    std::unique_ptr<DescriptorPool> m_DescriptorPool;
};


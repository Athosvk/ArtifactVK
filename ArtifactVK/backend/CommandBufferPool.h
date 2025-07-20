#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <span>
#include <functional>
#include <memory>
#include <string>

#include "Semaphore.h"
#include "Fence.h"
#include "Queue.h"
#include "Barrier.h"

class Framebuffer;
class RenderPass;
class RasterPipeline;
class VertexBuffer;
class UniformBuffer;
class DeviceBuffer;
class IndexBuffer;
class DescriptorSet;
class ExtensionFunctionMapping;
class VulkanInstance;

struct CommandBufferPoolCreateInfo
{
    VkCommandPoolCreateFlagBits CreationFlags;
    uint32_t QueueIndex;
};

class CommandBuffer
{
	enum class CommandBufferStatus
	{
		Recording,
		Submitted,
		Reset
	};

  public:
    CommandBuffer(VkCommandBuffer &&commandBuffer, VkDevice device, Queue queue);
    CommandBuffer(CommandBuffer && other);
    CommandBuffer(const CommandBuffer & other) = delete;
    ~CommandBuffer();

    void SetName(const std::string& name, const ExtensionFunctionMapping& functionMapping);
    void WaitFence();
    void Begin();
    void BeginSingleTake();
    void Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline, VertexBuffer& vertexBuffer, const DescriptorSet& descriptorSet);
    void DrawIndexed(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline, VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, const DescriptorSet& descriptorSet);
    Fence& End(std::span<Semaphore> waitSemaphores, std::span<Semaphore> signalSemaphores);
    Fence& End();
    void Copy(const DeviceBuffer &source, const DeviceBuffer &destination);
    void CopyBufferToImage(const DeviceBuffer& source, Texture& texture);
    void InsertBarrier(const BufferMemoryBarrier &barrier) const;
    void InsertBarrier(const ImageMemoryBarrier &barrier) const;
    void InsertBarriers(const BufferMemoryBarrierArray &barriers) const;
    Queue GetQueue() const;
  private:
    void BindVertexBuffer(VertexBuffer &vertexBuffer);
    void BindIndexBuffer(IndexBuffer &indexBuffer);
    void BindDescriptorSet(const DescriptorSet &uniformBuffer, const RasterPipeline& pipeline);
    void HandleAcquire(DeviceBuffer &buffer);
    void Reset();

    // Only used in case we Reset, which can clear a debug name previously
    // set.
    std::optional<std::string> m_Name;
    std::optional<std::reference_wrapper<const ExtensionFunctionMapping>> m_ExtensionFunctionMapping;
    bool m_Moved = false;
    VkDevice m_Device;
    VkCommandBuffer m_CommandBuffer;
    // TODO: Pool these fences in the CommandBufferPool
    // This is a shared ptr so that the fence can outlive (the ArtifactVK
    // representation of) the command buffer
    // TODO: But, do we really have to? We can probably map the construct
    // better so that End consumes into an executed command buffer,
    // possibly with a recyclable command buffer handle if it wasn't single
    // take
    // Needs to outlive the CommandBuffer in case it's moved
    std::unique_ptr<Fence> m_InFlight;
    CommandBufferStatus m_Status = CommandBufferStatus::Reset;
    Queue m_Queue;
    std::vector<BufferMemoryBarrierArray> m_PendingBarriers;
};

// TODO: Template with per-type command buffer, so that they only have the matching
// operations available
class CommandBufferPool
{
  public:
    CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo, const VulkanInstance& instance);
    CommandBufferPool(const CommandBufferPool &other) = delete;
    CommandBufferPool(CommandBufferPool &&other);
    ~CommandBufferPool();
    
    std::vector<std::reference_wrapper<CommandBuffer>> CreateCommandBuffers(uint32_t count, Queue queue);
    CommandBuffer& CreateCommandBuffer(Queue queue);
    void SetName(const std::string& name, ExtensionFunctionMapping mapping);
  private:
    const VulkanInstance &m_Instance;
    VkDevice m_Device;
    VkCommandPool m_CommandBufferPool;
    // TODO: Cleanup command buffers
    std::vector<std::unique_ptr<CommandBuffer>> m_CommandBuffers;
};

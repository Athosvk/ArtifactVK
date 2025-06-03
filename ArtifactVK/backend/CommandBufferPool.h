#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <span>
#include <functional>

#include "Semaphore.h"
#include "Fence.h"
#include "Queue.h"

class Framebuffer;
class RenderPass;
class RasterPipeline;
class VertexBuffer;
class DeviceBuffer;
class IndexBuffer;

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
    ~CommandBuffer();

    void WaitFence();
    void Begin();
    void Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline, const VertexBuffer& vertexBuffer);
    Fence& End(std::span<Semaphore> waitSemaphores, std::span<Semaphore> signalSemaphores);
    Fence& End();
    void BindVertexBuffer(const VertexBuffer &vertexBuffer);
    void BindIndexBuffer(const IndexBuffer &indexBuffer);
    void Copy(const DeviceBuffer &source, const DeviceBuffer &destination);
  private:
    void Reset();

    bool m_Moved = false;
    VkCommandBuffer m_CommandBuffer;
    // TODO: Pool these fences in the CommandBufferPool
    Fence m_InFlight;
    CommandBufferStatus m_Status = CommandBufferStatus::Reset;
    Queue m_Queue;
};

// TODO: Template with per-type command buffer, so that they only have the matching
// operations available
class CommandBufferPool
{
  public:
    CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo);
    CommandBufferPool(const CommandBufferPool &other) = delete;
    CommandBufferPool(CommandBufferPool &&other);
    ~CommandBufferPool();
    
    std::vector<std::reference_wrapper<CommandBuffer>> CreateCommandBuffers(uint32_t count, Queue queue);
    CommandBuffer& CreateCommandBuffer(Queue queue);
  private:
    VkDevice m_Device;
    VkCommandPool m_CommandBufferPool;
    // TODO: Cleanup command buffers
    std::vector<std::unique_ptr<CommandBuffer>> m_CommandBuffers;
};

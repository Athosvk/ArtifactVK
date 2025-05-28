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

struct CommandBufferPoolCreateInfo
{
    VkCommandPoolCreateFlagBits CreationFlags;
    uint32_t QueueIndex;
};

struct CommandBuffer
{
	enum class CommandBufferStatus
	{
		Recording,
		Submitted,
		Reset
	};

  public:
    CommandBuffer(VkCommandBuffer &&commandBuffer, VkDevice device);
    CommandBuffer(CommandBuffer && other);
    ~CommandBuffer();

    void WaitFence(bool log = false);
    void Begin();
    void Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline, const VertexBuffer& vertexBuffer);
    Fence& End(std::span<Semaphore> waitSemaphores, std::span<Semaphore> signalSemaphores, Queue queue);
    void BindBuffer(const VertexBuffer &vertexBuffer);
  private:
    void Reset();

    bool m_Moved = false;
    VkCommandBuffer m_CommandBuffer;
    // TODO: Pool these fences in the CommandBufferPool
    Fence m_InFlight;
    CommandBufferStatus m_Status = CommandBufferStatus::Reset;
};

class CommandBufferPool
{
  public:
    CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo);
    CommandBufferPool(const CommandBufferPool &other) = delete;
    CommandBufferPool(CommandBufferPool &&other);
    ~CommandBufferPool();
    
    std::vector<std::reference_wrapper<CommandBuffer>> CreateCommandBuffers(uint32_t count);
  private:
    VkDevice m_Device;
    VkCommandPool m_CommandBufferPool;
    std::vector<CommandBuffer> m_CommandBuffers;
};

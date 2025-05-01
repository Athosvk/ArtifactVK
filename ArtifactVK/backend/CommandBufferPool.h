#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <span>

#include "Semaphore.h"
#include "Fence.h"

class Framebuffer;
class RenderPass;
class RasterPipeline;

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
    CommandBuffer(CommandBuffer &&) = default;
    ~CommandBuffer();

    void Begin();
    void Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline);
    Fence& End(std::span<Semaphore> waitSemaphores, std::span<Semaphore> signalSemaphores, VkQueue queue);

  private:
    void Reset();

    VkCommandBuffer m_CommandBuffer;
    // TODO: Pool these fences in the CommandBufferPool
    Fence m_InFlight;
    CommandBufferStatus m_Status;
};

class CommandBufferPool
{
  public:
    CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo);
    CommandBufferPool(const CommandBufferPool &other) = delete;
    CommandBufferPool(CommandBufferPool &&other);
    ~CommandBufferPool();
    
    CommandBuffer &CreateCommandBuffer();
  private:
    VkDevice m_Device;
    VkCommandPool m_CommandBufferPool;
    std::vector<CommandBuffer> m_CommandBuffers;
};

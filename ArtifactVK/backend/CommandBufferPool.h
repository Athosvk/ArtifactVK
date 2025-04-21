#pragma once
#include <vulkan/vulkan.h>
#include <vector>

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
  public:
    CommandBuffer(VkCommandBuffer &&commandBuffer);
    void Begin();
    void Draw(const Framebuffer& frameBuffer, const RenderPass& renderPass, const RasterPipeline& pipeline);
  private:
    VkCommandBuffer m_CommandBuffer;
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

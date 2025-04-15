#pragma once

#include <vulkan/vulkan.h>

struct CommandBufferPoolCreateInfo
{
    VkCommandPoolCreateFlagBits CreationFlags;
    uint32_t QueueIndex;
};

class CommandBufferPool
{
  public:
    CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo);
    CommandBufferPool(const CommandBufferPool &other) = delete;
    CommandBufferPool(CommandBufferPool &&other);
    ~CommandBufferPool();

private:
    VkDevice m_Device;
    VkCommandPool m_CommandBufferPool;
};

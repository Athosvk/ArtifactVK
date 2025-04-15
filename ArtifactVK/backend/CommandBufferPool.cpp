#include "CommandBufferPool.h"
#include "VulkanDevice.h"

#include <cassert>
#include <vulkan/vulkan.h>
#include <stdexcept>

CommandBufferPool::CommandBufferPool(VkDevice device, CommandBufferPoolCreateInfo createInfo) : m_Device(device)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = createInfo.CreationFlags;
    commandPoolCreateInfo.queueFamilyIndex = createInfo.QueueIndex;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &m_CommandBufferPool))
    {
        throw std::runtime_error("Could not create CommandBufferPool for queue index " + std::to_string(createInfo.QueueIndex));
    }
}

CommandBufferPool::CommandBufferPool(CommandBufferPool &&other) : 
    m_Device(other.m_Device),
    m_CommandBufferPool(std::exchange(other.m_CommandBufferPool, VK_NULL_HANDLE))
{
}

CommandBufferPool::~CommandBufferPool()
{
    vkDestroyCommandPool(m_Device, m_CommandBufferPool, nullptr);
}

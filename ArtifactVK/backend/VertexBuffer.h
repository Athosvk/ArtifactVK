#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

#include "VulkanDevice.h"

template<typename T>
class VertexBuffer
{
  public:
    VertexBuffer(std::vector<T> data, VkDevice device, const VulkanDevice& physicalDevice)
    {
        VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = data.size() * sizeof(T);
        bufferCreateInfo.usage = VkBufferUsageFlags::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        // TODO: Allow the caller to specify
        bufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.flags = 0;

        if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &m_Buffer))
        {
            throw std::runtime_error("Could not create vertex buffer");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, m_Buffer, &memoryRequirements);
        auto typeIndex = physicalDevice.FindMemoryType(memoryRequirements.memoryTypeBits,
                                                   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                       VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        VkMemoryAllocateInfo allocationInfo;
        allocationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.memoryTypeIndex = typeIndex;
        allocationInfo.allocationSize = memoryRequirements.size;

        if (vkAllocateMemory(device, allocationInfo, nullptr, m_Memory))
        {
            throw std::runtime_error("Could not llocate vertex buffer");
        }
    }

    VertexBuffer(const VertexBuffer &) = delete;
    VertexBuffer(const VertexBuffer &&other) : m_Buffer(std::exchange(other.m_Buffer, VK_NULL_HANDLE)), m_Device(other.m_Device)
    {
    } 

    ~VertexBuffer()
    {
        vkDestroyBuffer(m_Device, m_Buffer, nullptr);
        vkFreeMemory(m_Device, m_Buffer, nullptr);
    }

    void Bind(uint32_t slot)
    {
        vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0);
    }


private:
    VkBuffer m_Buffer;
    VkDeviceMemory m_Memory;
    VkDevice m_Device;
};

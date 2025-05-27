#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

template<typename T>
class VertexBuffer
{
  public:
    VertexBuffer(std::vector<T> data, VkDevice device, const VkPhysicalDevice& physicalDevice)
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

    }

    VertexBuffer(const VertexBuffer &) = delete;
    VertexBuffer(const VertexBuffer &&other) : m_Buffer(std::exchange(other.m_Buffer, VK_NULL_HANDLE)), m_Device(other.m_Device)
    {
    } 

    ~VertexBuffer()
    {
        vkDestroyBuffer(m_Device, m_Buffer, nullptr);
    }


private:
    VkBuffer m_Buffer;
    VkDevice m_Device;
};

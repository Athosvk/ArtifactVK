#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

class PhysicalDevice;

class VertexBuffer
{
  public:
    template<typename T>
    VertexBuffer(std::vector<T> data, VkDevice device, const PhysicalDevice& physicalDevice) : 
        m_PhysicalDevice(physicalDevice),
        m_Device(device)
    {
        VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = data.size() * sizeof(T);
        bufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        // TODO: Allow the caller to specify
        bufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.flags = 0;

        if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &m_Buffer))
        {
            throw std::runtime_error("Could not create vertex buffer");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, m_Buffer, &memoryRequirements);
        auto typeIndex = FindMemoryType(memoryRequirements.memoryTypeBits,
                                                   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                       VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        VkMemoryAllocateInfo allocationInfo{};
        allocationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocationInfo.memoryTypeIndex = typeIndex;
        allocationInfo.allocationSize = memoryRequirements.size;

        if (vkAllocateMemory(device, &allocationInfo, nullptr, &m_Memory))
        {
            throw std::runtime_error("Could not llocate vertex buffer");
        }
    }

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) const;

    VertexBuffer(const VertexBuffer &) = delete;
    VertexBuffer(VertexBuffer &&other);
    ~VertexBuffer();

    void Bind();

private:
    VkBuffer m_Buffer;
    VkDeviceMemory m_Memory;
    VkDevice m_Device;
    const PhysicalDevice& m_PhysicalDevice;
};

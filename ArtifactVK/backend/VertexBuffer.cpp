#include "VertexBuffer.h"

#include "PhysicalDevice.h"

uint32_t VertexBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) const
{
    auto memoryProperties = m_PhysicalDevice.MemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) != 0
			&& (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
		{
			return i;
		}
	}
	throw std::runtime_error("Could not find suitable memory type for type filter: " + std::to_string(typeFilter));
}

VertexBuffer::VertexBuffer(VertexBuffer &&other)
    : m_Buffer(std::exchange(other.m_Buffer, VK_NULL_HANDLE)), m_Device(other.m_Device), m_PhysicalDevice(other.m_PhysicalDevice)
{
} 

VertexBuffer::~VertexBuffer()
{
	vkDestroyBuffer(m_Device, m_Buffer, nullptr);
	vkFreeMemory(m_Device, m_Memory, nullptr);
}

size_t VertexBuffer::VertexCount() const
{
    return m_VertexCount;
}

VkBuffer VertexBuffer::Get() const
{
    return m_Buffer;
}


#include <backend/UniformBuffer.h>

#include <backend/VulkanDevice.h>

UniformBuffer::UniformBuffer(VulkanDevice& vulkanDevice, VkDevice device, size_t bufferSize) 
	: m_Device(device),
	m_Buffer(CreateBuffer(vulkanDevice, bufferSize))
{
}

VkDescriptorBufferInfo UniformBuffer::GetDescriptorInfo() const
{
    return m_Buffer.GetDescriptorInfo();
}

DeviceBuffer &UniformBuffer::CreateBuffer(VulkanDevice &vulkanDevice, size_t size)
{
	CreateBufferInfo bufferInfo;
	bufferInfo.BufferUsage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.MemoryProperties = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferInfo.Size = size;

    return vulkanDevice.CreateBuffer(bufferInfo);
}

#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(CreateIndexBufferInfo bufferInfo, VkDevice device, const PhysicalDevice& physicalDevice, 
        CommandBuffer& transferCommandBuffer) : 
    m_StagingBuffer(CreateStagingBuffer(bufferInfo.InitialData.size() * sizeof(size_t), device, physicalDevice)),
      m_IndexBuffer(CreateIndexBuffer(bufferInfo.InitialData.size() * sizeof(size_t), device, physicalDevice))
{
}

DeviceBuffer IndexBuffer::CreateStagingBuffer(VkDeviceSize size, VkDevice device,
                                              const PhysicalDevice &physicalDevice) const
{

	auto stagingBufferInfo = CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    return DeviceBuffer(device, physicalDevice, stagingBufferInfo);
}

DeviceBuffer IndexBuffer::CreateIndexBuffer(VkDeviceSize size, VkDevice device,
                                            const PhysicalDevice &physicalDevice) const
{
	auto createIndexBufferInfo = CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    return DeviceBuffer(device, physicalDevice, createIndexBufferInfo);
}

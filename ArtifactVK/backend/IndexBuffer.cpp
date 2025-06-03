#include "IndexBuffer.h"
#include "CommandBufferPool.h"

IndexBuffer::IndexBuffer(CreateIndexBufferInfo bufferInfo, VkDevice device, const PhysicalDevice& physicalDevice, 
        CommandBuffer& transferCommandBuffer) : 
    m_StagingBuffer(CreateStagingBuffer(bufferInfo.InitialData.size() * sizeof(uint16_t), device, physicalDevice)),
      m_IndexBuffer(CreateIndexBuffer(bufferInfo.InitialData.size() * sizeof(uint16_t), device, physicalDevice))
{
	m_IndexCount = bufferInfo.InitialData.size();
	m_StagingBuffer.UploadData(bufferInfo.InitialData);
	transferCommandBuffer.Copy(m_StagingBuffer, m_IndexBuffer);
	// TODO: Use semaphore instead, allow fetching the semaphore
	m_TransferFence = transferCommandBuffer.End({}, {});

}

VkBuffer IndexBuffer::Get() const
{
    return m_IndexBuffer.Get();
}

size_t IndexBuffer::GetIndexCount() const
{
    return m_IndexCount;
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
	auto createIndexBufferInfo = CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
												   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    return DeviceBuffer(device, physicalDevice, createIndexBufferInfo);
}

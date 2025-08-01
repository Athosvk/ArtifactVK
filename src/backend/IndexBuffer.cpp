#include <backend/IndexBuffer.h>

#include <cassert>

#include <backend/CommandBufferPool.h>

IndexBuffer::IndexBuffer(CreateIndexBufferInfo bufferInfo, VkDevice device, const PhysicalDevice& physicalDevice, 
        CommandBuffer& transferCommandBuffer) : 
    m_StagingBuffer(CreateStagingBuffer(bufferInfo.InitialData.size() * sizeof(uint16_t), device, physicalDevice)),
      m_IndexBuffer(CreateIndexBuffer(bufferInfo.InitialData.size() * sizeof(uint16_t), device, physicalDevice))
{
    assert((bufferInfo.DestinationQueue.has_value() ^
               (bufferInfo.SharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT)) &&
           "Requires either a target queue or sharing mode to be set to VK_SHARING_MODE_CONCURRENT");
	m_IndexCount = bufferInfo.InitialData.size();
    m_StagingBuffer.UploadData(std::span<uint16_t>{bufferInfo.InitialData});
    transferCommandBuffer.BeginSingleTake();
	transferCommandBuffer.Copy(m_StagingBuffer, m_IndexBuffer);

	if (bufferInfo.SharingMode == VkSharingMode::VK_SHARING_MODE_EXCLUSIVE
		&& transferCommandBuffer.GetQueue().GetFamilyIndex() != bufferInfo.DestinationQueue->GetFamilyIndex())
	{
		m_StagingBuffer.Transfer(TransferOp{*bufferInfo.DestinationQueue,
											VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT,
											VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
												VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT}, transferCommandBuffer);
	}
	
	// TODO: Use semaphore instead, allow fetching the semaphore
	m_TransferFence = &transferCommandBuffer.End({}, {});
}

DeviceBuffer& IndexBuffer::GetBuffer()
{
    if (m_TransferFence != nullptr)
    {
		// TODO: Allow doing this explicitly instead, as we can't read
		// the intent behind calling `Get` this can lead to 
		// unexpected results
        m_TransferFence->WaitAndReset();
		m_TransferFence = nullptr;
	}
    return m_IndexBuffer;
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
						 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						false};
    return DeviceBuffer(device, physicalDevice, stagingBufferInfo);
}

DeviceBuffer IndexBuffer::CreateIndexBuffer(VkDeviceSize size, VkDevice device,
                                            const PhysicalDevice &physicalDevice) const
{
	auto createIndexBufferInfo = CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
												   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false};
    return DeviceBuffer(device, physicalDevice, createIndexBufferInfo);
}

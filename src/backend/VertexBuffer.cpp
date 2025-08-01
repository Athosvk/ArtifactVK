#include <backend/VertexBuffer.h>

#include <backend/PhysicalDevice.h>

size_t VertexBuffer::VertexCount() const
{
    return m_VertexCount;
}

DeviceBuffer& VertexBuffer::GetBuffer()
{
    if (m_TransferFence)
    {
		// TODO: Allow doing this explicitly instead, as we can't read
		// the intent behind calling `Get` this can lead to 
		// unexpected results
        m_TransferFence->WaitAndReset();   
		m_TransferFence = nullptr;
	}
    return m_VertexBuffer;
}

DeviceBuffer VertexBuffer::CreateStagingBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const
{
	auto createStagingBufferInfo =
		CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						false};
    return DeviceBuffer(device, physicalDevice, createStagingBufferInfo);
}

DeviceBuffer VertexBuffer::CreateVertexBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const
{
	auto createVertexBufferInfo = CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
												   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						false};
	return DeviceBuffer(device, physicalDevice, createVertexBufferInfo);
}


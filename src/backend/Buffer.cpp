#include <backend/Buffer.h>

#include <stdexcept>
#include <iostream>

#include <backend/PhysicalDevice.h>

DeviceBuffer::DeviceBuffer(VkDevice device, const PhysicalDevice &physicalDevice, CreateBufferInfo bufferInfo)
    : m_Device(device), m_CreateInfo(bufferInfo)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferInfo.Size;
	bufferCreateInfo.usage = bufferInfo.BufferUsage;
	bufferCreateInfo.sharingMode = bufferInfo.SharingMode;
	bufferCreateInfo.flags = 0;

	if (vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &m_Buffer))
	{
		throw std::runtime_error("Could not create vertex buffer");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memoryRequirements);
	auto typeIndex = physicalDevice.FindMemoryType(memoryRequirements.memoryTypeBits,
											   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
												   VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
												VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkMemoryAllocateInfo allocationInfo{};
	allocationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.memoryTypeIndex = typeIndex;
	allocationInfo.allocationSize = memoryRequirements.size;

	if (vkAllocateMemory(m_Device, &allocationInfo, nullptr, &m_Memory))
	{
		throw std::runtime_error("Could not allocate vertex buffer");
	}

	vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0);
    if (bufferInfo.PersistentlyMapped)
    {
        void *mappedBuffer;
        vkMapMemory(m_Device, m_Memory, 0, bufferInfo.Size, 0, &mappedBuffer);
        m_MappedBuffer.emplace(mappedBuffer);
	}
}

DeviceBuffer::DeviceBuffer(DeviceBuffer &&other)
    : m_Device(other.m_Device), m_Buffer(std::exchange(other.m_Buffer, VK_NULL_HANDLE)),
      m_Memory(std::exchange(other.m_Memory, VK_NULL_HANDLE)), m_CreateInfo(std::move(other.m_CreateInfo)),
      m_MappedBuffer(std::move(other.m_MappedBuffer))
{
}

DeviceBuffer &DeviceBuffer::operator=(DeviceBuffer &&other)
{
    m_Device = other.m_Device;
    m_Buffer = std::exchange(other.m_Buffer, VK_NULL_HANDLE);
    m_Memory = std::exchange(other.m_Memory, VK_NULL_HANDLE);
    m_CreateInfo = std::move(other.m_CreateInfo);
    m_MappedBuffer = std::move(other.m_MappedBuffer);
    return *this;
}

DeviceBuffer::~DeviceBuffer()
{
    if (m_MappedBuffer.has_value())
    {
        vkUnmapMemory(m_Device, m_Memory);
	}
	vkDestroyBuffer(m_Device, m_Buffer, nullptr);
	vkFreeMemory(m_Device, m_Memory, nullptr);
}

VkBuffer DeviceBuffer::Get() const
{
    return m_Buffer;
}

VkDeviceSize DeviceBuffer::GetSize() const
{
    return m_CreateInfo.Size;
}

void DeviceBuffer::Transfer(TransferOp transferOperation, const CommandBuffer& commandBuffer)
{
    assert(!m_PendingAcquireBarrier.has_value() && "Re-release before acquire");
    BufferMemoryBarrier releaseBarrier{
        (*this),
        {{commandBuffer.GetQueue(), transferOperation.Destination}},
        VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
        0,
        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    };
	commandBuffer.InsertBarrier(releaseBarrier);
	
    BufferMemoryBarrier acquireBarrier{
        (*this),
        {{commandBuffer.GetQueue(), transferOperation.Destination}},
        0,
        transferOperation.DestinationAccessMask,
        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        transferOperation.DestinationPipelineStage,
    };
    m_PendingAcquireBarrier.emplace(acquireBarrier);
}

std::optional<BufferMemoryBarrier> DeviceBuffer::TakePendingAcquire()
{
    if (m_PendingReleaseFence)
    {
        m_PendingReleaseFence->WaitAndReset();
    }
    return std::move(m_PendingAcquireBarrier);
}

VkDescriptorBufferInfo DeviceBuffer::GetDescriptorInfo() const
{
    VkDescriptorBufferInfo descriptorInfo{};
    descriptorInfo.buffer = m_Buffer;
    descriptorInfo.offset = 0;
    descriptorInfo.range = GetSize();
    return descriptorInfo;
}


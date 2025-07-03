#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <cassert>
#include <span>

#include "Barrier.h"

class PhysicalDevice;
class CommandBuffer;

struct CreateBufferInfo
{
    VkDeviceSize Size = 0xFFFFFFFF;
    VkBufferUsageFlags BufferUsage;
    VkMemoryPropertyFlags MemoryProperties;
    bool PersistentlyMapped = true;
    VkSharingMode SharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
};

struct TransferOp
{
    Queue Destination;
    VkAccessFlags DestinationAccessMask;
    VkPipelineStageFlags DestinationPipelineStage;
};

class DeviceBuffer
{
public:
	DeviceBuffer(VkDevice device, const PhysicalDevice& physicalDevice, CreateBufferInfo bufferInfo);
	DeviceBuffer(const DeviceBuffer &buffer) = delete;
	DeviceBuffer(DeviceBuffer &&buffer);
	~DeviceBuffer();

    VkBuffer Get() const;
    VkDeviceSize GetSize() const;
    void Transfer(TransferOp transferOperation, const CommandBuffer& commandBuffer);
    std::optional<BufferMemoryBarrier> TakePendingAcquire();

    template<typename T>
    void UploadData(std::span<T> data)
    {
        auto bufferSize = data.size() * sizeof(T);
        assert(bufferSize <= m_CreateInfo.Size && "Buffer too small for provided data");
        if (m_MappedBuffer.has_value())
        {
			memcpy(*m_MappedBuffer, data.data(), bufferSize);
        }
        else
        {
            void *mappedBuffer;
            vkMapMemory(m_Device, m_Memory, 0, bufferSize, 0, &mappedBuffer);
			memcpy(mappedBuffer, data.data(), bufferSize);
			vkUnmapMemory(m_Device, m_Memory);
        }
    }
    VkDescriptorBufferInfo GetDescriptorInfo() const;
private:
	VkDevice m_Device;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
    CreateBufferInfo m_CreateInfo;
    std::optional<void *> m_MappedBuffer;
    std::optional<BufferMemoryBarrier> m_PendingAcquireBarrier;
};

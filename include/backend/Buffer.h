#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <cassert>
#include <span>

#include "Barrier.h"

class PhysicalDevice;
class Fence;
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
	DeviceBuffer(const DeviceBuffer &) = delete;
	DeviceBuffer(DeviceBuffer &&buffer);
	DeviceBuffer& operator=(const DeviceBuffer &) = delete;
	DeviceBuffer& operator=(DeviceBuffer &&other);
	~DeviceBuffer();

    VkBuffer Get() const;
    VkDeviceSize GetSize() const;
    /// <summary>
    /// Transfer the buffer ownership to another queue (family)
    ///
    /// This assuems the most recent operation was an upload (i.e. `VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT`)
    /// </summary>
    /// <param name="transferOperation">The transfer operation parameters, specifying the context in which the buffer will be used on the target</param>
    /// <param name="commandBuffer">The (transfer) command buffer from which` queue (family) the buffer will be released</param>
    void Transfer(TransferOp transferOperation, const CommandBuffer& commandBuffer);

    /// <summary>
    /// Takes the transfer acquire barrier, if there is any, for a previously enqueued release barrier used for uploading data
    /// </summary>
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
    Fence *m_PendingReleaseFence = nullptr;
};

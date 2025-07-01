#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>
#include <optional>

#include "Buffer.h"
#include "CommandBufferPool.h"

class PhysicalDevice;

template<typename T>
struct CreateVertexBufferInfo
{
    std::vector<T> InitialData;
    VkSharingMode SharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    std::optional<Queue> DestinationQueue;
};

class VertexBuffer
{
  public:
    template<typename T>
    VertexBuffer(CreateVertexBufferInfo<T> bufferInfo, VkDevice device, const PhysicalDevice& physicalDevice, 
            // TODO: Optional so that you don't have to opt in to the copying to device local
            CommandBuffer& transferCommandBuffer) : 
        m_PhysicalDevice(physicalDevice), m_VertexCount(bufferInfo.InitialData.size()),
	    m_StagingBuffer(CreateStagingBuffer(bufferInfo.InitialData.size() * sizeof(T), device, physicalDevice)),
	    m_VertexBuffer(CreateVertexBuffer(bufferInfo.InitialData.size() * sizeof(T), device, physicalDevice))
    {

        assert(bufferInfo.DestinationQueue.has_value() ^
               (bufferInfo.SharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT) &&
           "Requires either a target queue or sharing mode to be set to VK_SHARING_MODE_CONCURRENT");
        m_VertexCount = bufferInfo.InitialData.size();
        m_StagingBuffer.UploadData(bufferInfo.InitialData);
        transferCommandBuffer.Copy(m_StagingBuffer, m_VertexBuffer);
        if (bufferInfo.SharingMode == VkSharingMode::VK_SHARING_MODE_EXCLUSIVE
            && transferCommandBuffer.GetQueue().GetFamilyIndex() != bufferInfo.DestinationQueue->GetFamilyIndex())
        {
            m_StagingBuffer.Transfer(TransferOp{*bufferInfo.DestinationQueue,
                                                VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                                                VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
                                                    VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
                transferCommandBuffer);
        }
        // TODO: Use semaphore instead, allow fetching the semaphore
        m_TransferFence = transferCommandBuffer.End({}, {});
    }

    size_t VertexCount() const;
    DeviceBuffer& GetBuffer();
  private:
    DeviceBuffer CreateStagingBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;
    DeviceBuffer CreateVertexBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;

    const PhysicalDevice& m_PhysicalDevice;
    DeviceBuffer m_StagingBuffer;
    DeviceBuffer m_VertexBuffer;
    size_t m_VertexCount;
    std::optional<std::reference_wrapper<Fence>> m_TransferFence;
};

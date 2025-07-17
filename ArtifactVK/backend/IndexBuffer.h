#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <memory>

#include "Buffer.h"
#include "Fence.h"

class CommandBuffer;
class PhysicalDevice;

struct CreateIndexBufferInfo
{
    // TODO: User configure for uint32_t
    std::vector<uint16_t> InitialData;
    VkSharingMode SharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    std::optional<Queue> DestinationQueue;
};

class IndexBuffer
{
  public:
    IndexBuffer(CreateIndexBufferInfo bufferInfo, VkDevice device, const PhysicalDevice& physicalDevice, 
        // TODO: Optional so that you don't have to opt in to the copying to device local
        CommandBuffer& transferCommandBuffer);

    DeviceBuffer& GetBuffer();
    size_t GetIndexCount() const;
  private:
    DeviceBuffer CreateStagingBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;
    DeviceBuffer CreateIndexBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;

    DeviceBuffer m_StagingBuffer;
    DeviceBuffer m_IndexBuffer;
    size_t m_IndexCount;

    std::shared_ptr<Fence> m_TransferFence;
};

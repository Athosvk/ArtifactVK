#pragma once
#include <vulkan/vulkan.h>

#include "Buffer.h"

class CommandBuffer;
class PhysicalDevice;

struct CreateIndexBufferInfo
{
    std::vector<size_t> InitialData;
    VkBufferUsageFlags Flags = 0;
    VkSharingMode SharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
};

class IndexBuffer
{
  public:
    IndexBuffer(CreateIndexBufferInfo bufferInfo, VkDevice device, const PhysicalDevice& physicalDevice, 
        // TODO: Optional so that you don't have to opt in to the copying to device local
        CommandBuffer& transferCommandBuffer);

  private:
    DeviceBuffer CreateStagingBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;
    DeviceBuffer CreateIndexBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;

    DeviceBuffer m_StagingBuffer;
    DeviceBuffer m_IndexBuffer;
};

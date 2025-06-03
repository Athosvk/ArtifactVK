#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

#include "Buffer.h"
#include "CommandBufferPool.h"

class PhysicalDevice;

template<typename T>
struct CreateVertexBufferInfo
{
    std::vector<T> InitialData;
    VkBufferUsageFlags Flags = 0;
    VkSharingMode SharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
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
        m_VertexCount = bufferInfo.InitialData.size();
        m_StagingBuffer.UploadData(bufferInfo.InitialData);
    }

    size_t VertexCount() const;

    VkBuffer Get() const;
  private:
    DeviceBuffer CreateStagingBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;
    DeviceBuffer CreateVertexBuffer(VkDeviceSize size, VkDevice device, const PhysicalDevice& physicalDevice) const;

    const PhysicalDevice& m_PhysicalDevice;
    DeviceBuffer m_StagingBuffer;
    DeviceBuffer m_VertexBuffer;
    size_t m_VertexCount;
};

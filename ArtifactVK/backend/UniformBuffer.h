#pragma once
#include "Buffer.h"

#include <stdexcept>

class VulkanDevice;

class UniformBuffer
{
public:
    UniformBuffer(VulkanDevice& vulkanDevice, VkDevice device, size_t bufferSize);
    UniformBuffer(UniformBuffer &&other) = default;
    UniformBuffer(const UniformBuffer &other) = delete;

    template<typename T> void UploadData(const T &data)
    {
        m_Buffer.UploadData(std::span<const T>(&data, 1));
    }

    VkDescriptorBufferInfo GetDescriptorInfo() const;
  private:
    DeviceBuffer& CreateBuffer(VulkanDevice& vulkanDevice, size_t size);

    VkDevice m_Device;
    DeviceBuffer& m_Buffer;
};

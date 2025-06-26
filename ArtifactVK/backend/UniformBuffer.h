#pragma once
#include "Buffer.h"

#include <stdexcept>

class VulkanDevice;

class UniformBuffer
{
public:
    UniformBuffer(VulkanDevice& vulkanDevice, VkDevice device, size_t bufferSize, VkDescriptorSetLayout descriptorSetLayout);
    UniformBuffer(UniformBuffer &&other) = default;
    UniformBuffer(const UniformBuffer &other) = delete;


    VkDescriptorSetLayout GetDescriptorSetLayout() const;
    template<typename T> void UploadData(const T &data)
    {
        m_Buffer.UploadData(std::span<const T>(&data, 1));
    }

    void SetDescriptorSet(VkDescriptorSet descriptorSet);
    VkDescriptorSet GetDescriptorSet() const;
    VkDescriptorBufferInfo GetDescriptorInfo() const;
  private:
    DeviceBuffer& CreateBuffer(VulkanDevice& vulkanDevice, size_t size);

    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkDevice m_Device;
    DeviceBuffer& m_Buffer;
    // TODO: Temporary, shouldn't be controlled by this
    VkDescriptorSet m_DescriptorSet;
};

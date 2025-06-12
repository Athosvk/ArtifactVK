#pragma once
#include "Buffer.h"

#include <stdexcept>

class VulkanDevice;

class UniformBuffer
{
public:
    UniformBuffer(VulkanDevice& vulkanDevice, VkDevice device, size_t bufferSize, VkDescriptorSetLayout descriptorSetLayout);

    VkDescriptorSetLayout GetDescriptorSetLayout() const;
  private:
    DeviceBuffer& CreateBuffer(VulkanDevice& vulkanDevice, size_t size);

    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkDevice m_Device;
    DeviceBuffer& m_Buffer;
	void* m_MappedData;
};

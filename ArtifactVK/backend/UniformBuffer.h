#pragma once
#include "Buffer.h"

#include <stdexcept>

class VulkanDevice;

class UniformBuffer
{
public:
    UniformBuffer(VulkanDevice& vulkanDevice, VkDevice device, size_t bufferSize);

private:
    DeviceBuffer& CreateBuffer(VulkanDevice& vulkanDevice, size_t size);

  private:
    VkDevice m_Device;
    VkDescriptorSetLayout m_DescriptorSet;
    DeviceBuffer& m_Buffer;
	void* m_MappedData;
};

#pragma once
#include <cstdint>

#include <vulkan/vulkan.h>

class UniformBuffer;

struct DescriptorPoolCreateInfo
{
    uint32_t Size = 4096;
};

class DescriptorPool
{
  public:
    DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& descriptorPoolCreateInfo);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&&);

    VkDescriptorSet CreateDescriptorSet(const UniformBuffer& uniformBuffer);
private:
    VkDescriptorPool m_DescriptorPool;
    VkDevice m_Device;
};

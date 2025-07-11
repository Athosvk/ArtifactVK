#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

class UniformBuffer;

struct DescriptorPoolCreateInfo
{
    uint32_t SizePerType = 4096;
    std::vector<VkDescriptorType> Types;
};

class DescriptorPool
{
  public:
    DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& descriptorPoolCreateInfo);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&&) = default;

    VkDescriptorSet CreateDescriptorSet(VkDescriptorSetLayout layout);
private:
    std::unordered_map<VkDescriptorType, VkDescriptorPool> m_DescriptorPools;
    VkDevice m_Device;
};

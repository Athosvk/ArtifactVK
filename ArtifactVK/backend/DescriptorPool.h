#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

class UniformBuffer;

struct DescriptorPoolCreateInfo
{
    uint32_t SizePerType = 4096;
    // TODO: Convert to custom flags to prevent allocations
    std::vector<VkDescriptorType> Types;
};

class DescriptorPool
{
  public:
    DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& descriptorPoolCreateInfo);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool(DescriptorPool&& other);

    VkDescriptorSet CreateDescriptorSet(VkDescriptorSetLayout layout);
private:
    VkDescriptorPool m_DescriptorPool;
    VkDevice m_Device;
};

#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "DescriptorSetBuilder.h"

class UniformBuffer;
class DescriptorSetLayout;

struct DescriptorPoolCreateInfo
{
    uint32_t SizePerType = 4;
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

    DescriptorSet CreateDescriptorSet(const DescriptorSetLayout& layout);
private:
    VkDescriptorPool m_DescriptorPool;
    VkDevice m_Device;
};

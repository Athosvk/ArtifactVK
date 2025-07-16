#include "DescriptorPool.h"
#include "UniformBuffer.h"

#include <stdexcept>

DescriptorPool::DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo &descriptorPoolCreateInfo) 
    : m_Device(device)
{
    // TODO: Might not need a Vector here
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(descriptorPoolCreateInfo.Types.size());
    for (auto descriptorType : descriptorPoolCreateInfo.Types)
    {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = descriptorType;
		poolSize.descriptorCount = static_cast<uint32_t>(descriptorPoolCreateInfo.SizePerType);
        poolSizes.emplace_back(poolSize);
    }
		
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(descriptorPoolCreateInfo.SizePerType);

	if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

DescriptorPool::~DescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    }
}

DescriptorPool::DescriptorPool(DescriptorPool && other) : 
    m_DescriptorPool(std::exchange(other.m_DescriptorPool, VK_NULL_HANDLE)), 
    m_Device(other.m_Device)
{
}

VkDescriptorSet DescriptorPool::CreateDescriptorSet(VkDescriptorSetLayout layout)
{
    // TODO: Allocate for all uniform buffers at once
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
    return descriptorSet;
}

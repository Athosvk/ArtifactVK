#include "DescriptorPool.h"
#include "UniformBuffer.h"

#include <stdexcept>

DescriptorPool::DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo &descriptorPoolCreateInfo) 
    : m_Device(device)
{
    m_DescriptorPools.reserve(descriptorPoolCreateInfo.Types.size());
    for (auto descriptorType : descriptorPoolCreateInfo.Types)
    {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(descriptorPoolCreateInfo.SizePerType);
		
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;

		// TODO: Incorrect?
		poolInfo.maxSets = static_cast<uint32_t>(descriptorPoolCreateInfo.SizePerType);

        VkDescriptorPool pool;
		if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &pool) != VkResult::VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor pool");
		}
        m_DescriptorPools.emplace(descriptorType, pool);
    }
}

DescriptorPool::~DescriptorPool()
{
    for (auto [poolType, pool] : m_DescriptorPools) 
    {
        vkDestroyDescriptorPool(m_Device, pool, nullptr);
    }
}

VkDescriptorSet DescriptorPool::CreateDescriptorSet(VkDescriptorSetLayout layout)
{
    // TODO: Allocate for all uniform buffers at once
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPools[VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER];
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
    return descriptorSet;
}

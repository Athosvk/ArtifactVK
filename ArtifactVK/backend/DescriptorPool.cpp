#include "DescriptorPool.h"
#include "UniformBuffer.h"

#include <stdexcept>

DescriptorPool::DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo &descriptorPoolCreateInfo) 
    : m_Device(device)
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(descriptorPoolCreateInfo.Size);
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    // TODO: Incorrect?
    poolInfo.maxSets = static_cast<uint32_t>(descriptorPoolCreateInfo.Size);

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
    m_DescriptorPool(std::exchange(other.m_DescriptorPool, VK_NULL_HANDLE)), m_Device(other.m_Device)
{
}

VkDescriptorSet DescriptorPool::CreateDescriptorSet(const UniformBuffer& uniformBuffer)
{
    // TODO: Allocate for all uniform buffers at once
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    VkDescriptorSetLayout layout = uniformBuffer.GetDescriptorSetLayout();
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
    VkWriteDescriptorSet descriptorWriteInfo{};
    descriptorWriteInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteInfo.dstSet = descriptorSet;
    descriptorWriteInfo.dstBinding = 0;

    descriptorWriteInfo.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // TODO: Support array bindings
    descriptorWriteInfo.dstArrayElement = 0;
    descriptorWriteInfo.descriptorCount = 1;

    VkDescriptorBufferInfo bufferInfo = uniformBuffer.GetDescriptorInfo();
    descriptorWriteInfo.pBufferInfo = &bufferInfo;
    descriptorWriteInfo.pImageInfo = nullptr;
    descriptorWriteInfo.pTexelBufferView = nullptr;
    vkUpdateDescriptorSets(m_Device, 1, &descriptorWriteInfo, 0, nullptr);
    return descriptorSet;
}

#include "DescriptorSetBuilder.h"

#include "backend/UniformBuffer.h"
#include "backend/Texture.h"
#include "backend/DescriptorPool.h"

BindSet::BindSet(const DescriptorSet &descriptorSet, VkDevice device) : 
    m_DescriptorSet(descriptorSet),
    m_Device(device)
{
}

BindSet &BindSet::BindTexture(const Texture &texture)
{
	// TODO
    return *this;
}

BindSet &BindSet::BindUniformBuffer(const UniformBuffer& buffer)
{
	VkWriteDescriptorSet descriptorWriteInfo{};
	descriptorWriteInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriteInfo.dstSet = m_DescriptorSet.Get();
	descriptorWriteInfo.dstBinding = 0;

	descriptorWriteInfo.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// TODO: Support array bindings
	descriptorWriteInfo.dstArrayElement = 0;
	descriptorWriteInfo.descriptorCount = 1;

	VkDescriptorBufferInfo bufferInfo = buffer.GetDescriptorInfo();
	descriptorWriteInfo.pBufferInfo = &bufferInfo;
	descriptorWriteInfo.pImageInfo = nullptr;
	descriptorWriteInfo.pTexelBufferView = nullptr;
    m_StagingDescriptorSetWrites.emplace_back(buffer);
    return *this;
}

void BindSet::Finish()
{
    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(m_StagingDescriptorSetWrites.size()),
		m_StagingDescriptorSetWrites.data(), 0, nullptr);
}

VkDescriptorSet DescriptorSet::Get() const
{
    return m_DescriptorSet;
}

BindSet DescriptorSet::BindTexture(const Texture& texture)
{
    return BindSet(*this, m_Device).BindTexture(texture);
}

BindSet DescriptorSet::BindUniformBuffer(const UniformBuffer& buffer)
{
    return BindSet(*this, m_Device).BindUniformBuffer(buffer);
}

DescriptorSet::DescriptorSet(VkDescriptorSetLayout layout, VkDevice device, VkDescriptorSet set) : 
	m_Layout(layout), m_Device(device), m_DescriptorSet(set)
{
}

DescriptorSet::DescriptorSet(DescriptorSet && other) : 
	m_Layout(std::exchange(other.m_Layout, VK_NULL_HANDLE)), m_Device(other.m_Device), 
	m_DescriptorSet(std::move(other.m_DescriptorSet))
{
}

DescriptorSet::~DescriptorSet()
{
    vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
	// TODO: Free m_DescriptorSet;
}

DescriptorSetBuilder &DescriptorSetBuilder::AddUniformBuffer()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = m_Bindings.size();
	uboLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT |
                                  VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT |
                                  VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;  

	m_Bindings.emplace_back(uboLayoutBinding);
    return *this;
}

DescriptorSetBuilder &DescriptorSetBuilder::AddTexture()
{
	VkDescriptorSetLayoutBinding textureLayoutBinding{};
	textureLayoutBinding.binding = m_Bindings.size();
	textureLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.descriptorCount = 1;
    textureLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT |
                                      VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT |
                                      VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
	textureLayoutBinding.pImmutableSamplers = nullptr;  

	m_Bindings.emplace_back(textureLayoutBinding);
    return *this;
}

DescriptorSet DescriptorSetBuilder::Build(DescriptorPool& pool, VkDevice device)
{
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = m_Bindings.size();
	createInfo.pBindings = m_Bindings.data();
		
	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout) != VkResult::VK_SUCCESS) {
		throw std::runtime_error("Could not create descriptor set for uniform buffer");
	}
    return DescriptorSet{descriptorSetLayout, device, pool.CreateDescriptorSet(descriptorSetLayout)};
}

#include "DescriptorSetBuilder.h"

#include "UniformBuffer.h"
#include "Texture.h"
#include "DescriptorPool.h"
#include "DebugMarker.h"

BindSet::BindSet(const DescriptorSet &descriptorSet, VkDevice device) : 
    m_DescriptorSet(descriptorSet),
    m_Device(device)
{
}

BindSet::BindSet(BindSet &&other)
    : m_DescriptorSet(other.m_DescriptorSet),
      m_StagingDescriptorSetWrites(std::move(other.m_StagingDescriptorSetWrites)), m_Device(other.m_Device),
      m_FinishedOrMoved(false)
{
    other.m_FinishedOrMoved = true;
}

BindSet::~BindSet()
{
    assert(m_FinishedOrMoved && "Did not call Finish on a bindset. Any bind actions were discarded");
}

BindSet &BindSet::operator=(BindSet && other)
{
    m_DescriptorSet = other.m_DescriptorSet;
    m_StagingDescriptorSetWrites = std::move(other.m_StagingDescriptorSetWrites);
    m_Device = other.m_Device;
    m_FinishedOrMoved = false;
    return *this;
}

BindSet &BindSet::BindTexture(const Texture &texture)
{
	// TODO: Verify which slot this goes into with original layout
	// TODO
    return *this;
}

BindSet &BindSet::BindUniformBuffer(const UniformBuffer& buffer)
{
	// TODO: Verify which slot this goes into with original layout
	VkWriteDescriptorSet descriptorWriteInfo{};
	descriptorWriteInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriteInfo.dstSet = m_DescriptorSet.get().Get();
	descriptorWriteInfo.dstBinding = 0;

	descriptorWriteInfo.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// TODO: Support array bindings
	descriptorWriteInfo.dstArrayElement = 0;
	descriptorWriteInfo.descriptorCount = 1;

	// Just need to keep this alive up to the point at which we bulk write
    auto& bufferInfo = m_BufferInfos.emplace_back(std::make_unique<VkDescriptorBufferInfo>(buffer.GetDescriptorInfo()));

	descriptorWriteInfo.pBufferInfo = bufferInfo.get();
	descriptorWriteInfo.pImageInfo = nullptr;
	descriptorWriteInfo.pTexelBufferView = nullptr;
    m_StagingDescriptorSetWrites.emplace_back(descriptorWriteInfo);
    return *this;
}

void BindSet::Finish()
{
    m_FinishedOrMoved = true;
    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(m_StagingDescriptorSetWrites.size()),
		m_StagingDescriptorSetWrites.data(), 0, nullptr);
}

VkDescriptorSet DescriptorSet::Get() const
{
    return m_DescriptorSet;
}

const DescriptorSetLayout& DescriptorSet::GetLayout() const
{
    return m_Layout;
}

void DescriptorSet::SetName(const std::string &name, const ExtensionFunctionMapping& mapping)
{
    DebugMarker::SetName(m_Device, mapping, m_DescriptorSet, name);
}

DescriptorSet::DescriptorSet(const DescriptorSetLayout &layout, VkDevice device, VkDescriptorSet set)
    : m_Layout(layout), m_Device(device), m_DescriptorSet(set)
{
}

BindSet DescriptorSet::BindTexture(const Texture &texture)
{
    auto bindset = BindSet(*this, m_Device);
    bindset.BindTexture(texture);
    return bindset;
}

BindSet DescriptorSet::BindUniformBuffer(const UniformBuffer& buffer)
{
    auto bindset = BindSet(*this, m_Device);
    bindset.BindUniformBuffer(buffer);
    return bindset;
}


VkDescriptorSetLayout DescriptorSetLayout::Get() const
{
    return m_Layout;
}

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings) : 
    m_Device(device), m_Bindings(std::move(bindings))
{
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	// TODO: Consider a small vector or even array, as this hould be at most four bindings.
	createInfo.bindingCount = static_cast<uint32_t>(m_Bindings.size());
	createInfo.pBindings = m_Bindings.data();
		
    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_Layout) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create descriptor set for uniform buffer");
    }
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (m_Layout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
    }
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other)
    : m_Layout(std::exchange(other.m_Layout, VK_NULL_HANDLE)), m_Device(other.m_Device), m_Bindings(std::move(other.m_Bindings))
{
}

DescriptorSetBuilder &DescriptorSetBuilder::AddUniformBuffer()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = static_cast<uint32_t>(m_Bindings.size());
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
	textureLayoutBinding.binding = static_cast<uint32_t>(m_Bindings.size());
	textureLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.descriptorCount = 1;
    textureLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT |
                                      VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT |
                                      VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
	textureLayoutBinding.pImmutableSamplers = nullptr;  

	m_Bindings.emplace_back(textureLayoutBinding);
    return *this;
}

DescriptorSetLayout DescriptorSetBuilder::Build(VkDevice device)
{
    return DescriptorSetLayout{device, std::move(m_Bindings)};
}


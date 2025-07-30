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

BindSet& BindSet::BindTexture(Texture2D &texture) &
{
    BindTextureInternal(texture);
    return *this;
}

BindSet& BindSet::BindUniformBuffer(const UniformBuffer& buffer) &
{
    BindUniformBufferInternal(buffer);
    return *this;
}

BindSet&& BindSet::BindTexture(Texture2D &texture) &&
{
    BindTextureInternal(texture);
    return std::move(*this);
}

BindSet&& BindSet::BindUniformBuffer(const UniformBuffer& buffer) &&
{
    BindUniformBufferInternal(buffer);
    return std::move(*this);
}

void BindSet::BindTextureInternal(Texture2D &texture)
{
	// TODO: Verify which slot this goes into with original layout
	VkWriteDescriptorSet descriptorWriteInfo{};
	descriptorWriteInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriteInfo.dstSet = m_DescriptorSet.Get();
	descriptorWriteInfo.dstBinding = static_cast<uint32_t>(m_Entries.size());

	descriptorWriteInfo.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// TODO: Support array bindings
	descriptorWriteInfo.dstArrayElement = 0;
	descriptorWriteInfo.descriptorCount = 1;

    // We fill this in once we combine all the writes into one invocation!
	descriptorWriteInfo.pImageInfo = nullptr;
    m_Entries.emplace_back(BindEntry{descriptorWriteInfo, {.ImageInfo = texture.GetDescriptorInfo()}});
    auto pendingAcquire = texture.TakePendingAcquire();
    if (pendingAcquire.has_value())
    {
        m_PendingAcquires.emplace_back(*pendingAcquire);
    }
}

void BindSet::BindUniformBufferInternal(const UniformBuffer &buffer)
{
	// TODO: Verify which slot this goes into with original layout
	VkWriteDescriptorSet descriptorWriteInfo{};
	descriptorWriteInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriteInfo.dstSet = m_DescriptorSet.Get();
	descriptorWriteInfo.dstBinding = static_cast<uint32_t>(m_Entries.size());

	descriptorWriteInfo.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// TODO: Support array bindings
	descriptorWriteInfo.dstArrayElement = 0;
	descriptorWriteInfo.descriptorCount = 1;

    // We fill this in once we combine all the writes into one invocation!
	descriptorWriteInfo.pBufferInfo = nullptr;
    m_Entries.emplace_back(BindEntry{descriptorWriteInfo, {.BufferInfo = buffer.GetDescriptorInfo()}});
}


void BindSet::FlushWrites()
{
    // TODO: Split the vectors so that we don't have to copy
    // into this vecotr at the point of submission.
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(m_Entries.size());
    for (auto& entry : m_Entries)
    {
        // We take the buffer info addresses here, but we're not 
        // modifying the vector length prior to submission, so at this 
        // point it's safe
        switch (entry.StagingDescriptorWrite.descriptorType)
        {
        case VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            entry.StagingDescriptorWrite.pBufferInfo = &entry.DataInfo.BufferInfo;
            break;
        case VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            entry.StagingDescriptorWrite.pImageInfo = &entry.DataInfo.ImageInfo;
            entry.StagingDescriptorWrite.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(&entry.DataInfo.ImageInfo);
            break;
        default:
            assert(false && "Unhandled descriptor type");
        }
        writes.emplace_back(entry.StagingDescriptorWrite);
    }
    // Note that we don't have to flush any pending acquires here, since
    // vkUpdateDescriptorSets doesn't actually access the memory. As long
    // as we flush them before we bind the descriptor sets, this is fine.
    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(writes.size()),
		writes.data(), 0, nullptr);
}

std::vector<ImageMemoryBarrier> BindSet::TakePendingAcquires()
{
    return std::move(m_PendingAcquires);
}

const DescriptorSet &BindSet::GetDescriptorSet() const
{
    return m_DescriptorSet;
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

BindSet DescriptorSet::BindTexture(Texture2D &texture)
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


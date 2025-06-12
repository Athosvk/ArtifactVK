#include "UniformBuffer.h"

#include "VulkanDevice.h"

UniformBuffer::UniformBuffer(VulkanDevice& vulkanDevice, VkDevice device, size_t bufferSize) 
	: m_Device(device),
	m_Buffer(CreateBuffer(vulkanDevice, bufferSize))
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;  

	VkDescriptorSetLayoutCreateInfo createInfo;
	createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1;
	createInfo.pBindings = &uboLayoutBinding;
		
	if (vkCreateDescriptorSetLayout(m_Device, &createInfo, nullptr, &m_DescriptorSet) != VkResult::VK_SUCCESS) {
		throw std::runtime_error("Could not create descriptor set for uniform buffer");
	}
}

DeviceBuffer& UniformBuffer::CreateBuffer(VulkanDevice &vulkanDevice, size_t size)
{
	CreateBufferInfo bufferInfo;
	bufferInfo.BufferUsage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.MemoryProperties = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferInfo.Size = size;

    return vulkanDevice.CreateBuffer(bufferInfo);
}

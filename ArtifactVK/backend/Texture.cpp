#include "Texture.h"

VkDeviceSize TextureCreateInfo::BufferSize() const
{
	// TODO: Fix size for varying channels
    return Width * Height * 4;
}

Texture::Texture( VkDevice device, const PhysicalDevice &physicalDevice, const TextureCreateInfo &textureCreateInfo) : 
	m_StagingBuffer(CreateStagingBuffer(textureCreateInfo.BufferSize(), physicalDevice, device))
{
    m_StagingBuffer.UploadData(textureCreateInfo.Pixels);
}

DeviceBuffer Texture::CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const
{
	auto createStagingBufferInfo =
		CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							 VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						false};
    return DeviceBuffer(device, physicalDevice, createStagingBufferInfo);
}


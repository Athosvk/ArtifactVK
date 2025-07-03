#include "Texture.h"

#include <stdexcept>

#include "PhysicalDevice.h"

VkDeviceSize TextureCreateInfo::BufferSize() const
{
	// TODO: Fix size for varying channels
    return Width * Height * 4;
}

Texture::Texture(VkDevice device, const PhysicalDevice &physicalDevice, const TextureCreateInfo &textureCreateInfo, CommandBuffer& transferCommandBuffer) :
	m_StagingBuffer(CreateStagingBuffer(textureCreateInfo.BufferSize(), physicalDevice, device))
{
    m_StagingBuffer.UploadData(textureCreateInfo.Pixels);
    VkImageCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
    vkCreateInfo.extent = {textureCreateInfo.Width, textureCreateInfo.Height, 1};
    vkCreateInfo.mipLevels = 1;
    vkCreateInfo.arrayLayers = 1;

    vkCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
    vkCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
    vkCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateInfo.usage =
        VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
    vkCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &vkCreateInfo, nullptr, &m_Image) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create iamge");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, m_Image, &memoryRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = physicalDevice.FindMemoryType(
        memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not allocate texture memory");
    }

    vkBindImageMemory(device, m_Image, m_Memory, 0);
    transferCommandBuffer.Begin();
}

Texture::~Texture()
{
    vkDestroyImage(m_Device, m_Image, nullptr);
    vkFreeMemory(m_Device, m_Memory, nullptr);
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


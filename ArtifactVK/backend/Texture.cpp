#include "Texture.h"

#include <stdexcept>

#include "PhysicalDevice.h"

VkDeviceSize TextureCreateInfo::BufferSize() const
{
	// TODO: Fix size for varying channels
    return Width * Height * 4;
}

Texture::Texture(VkDevice device, const PhysicalDevice &physicalDevice, const TextureCreateInfo &textureCreateInfo, CommandBuffer& transferCommandBuffer,
    Queue destinationQueue) : 
    m_Device(device),
    m_StagingBuffer(CreateStagingBuffer(textureCreateInfo.BufferSize(), physicalDevice, device)),
    m_Width(textureCreateInfo.Width),
    m_Height(textureCreateInfo.Height)
{
    m_StagingBuffer.UploadData(textureCreateInfo.Data);
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
    vkCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

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

    transferCommandBuffer.BeginSingleTake();
    TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     transferCommandBuffer,
                     std::nullopt);
    transferCommandBuffer.CopyBufferToImage(m_StagingBuffer, *this);

    TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, transferCommandBuffer, destinationQueue);
    m_PendingTransferFence = &transferCommandBuffer.End();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture image view");
    }
    CreateTextureSampler(device, physicalDevice);
}

Texture::Texture(Texture && other) : 
    m_Device(other.m_Device), m_StagingBuffer(std::move(other.m_StagingBuffer)), 
    m_Image(std::exchange(other.m_Image, VK_NULL_HANDLE)), m_Memory(std::exchange(other.m_Memory, VK_NULL_HANDLE)),
    m_Width(other.m_Width), m_Height(other.m_Height), 
    m_PendingAcquireBarrier(std::move(other.m_PendingAcquireBarrier)), 
    m_PendingTransferFence(other.m_PendingTransferFence), m_ImageView(std::exchange(other.m_ImageView, VK_NULL_HANDLE)), 
    m_Sampler(std::exchange(other.m_Sampler, VK_NULL_HANDLE))
{  
}

Texture::~Texture()
{
    if (m_Image != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
        vkDestroyImage(m_Device, m_Image, nullptr);
        vkDestroySampler(m_Device, m_Sampler, nullptr);
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }
}

VkImage Texture::Get()
{
    // TODO: Allow doing this explicitly instead, as we can't read
    // the intent behind calling `Get` this can lead to
    // unexpected results
    WaitTransfer();
    return m_Image;
}

uint32_t Texture::GetWidth() const
{
    return m_Width;
}

uint32_t Texture::GetHeight() const
{
    return m_Height;
}

VkDescriptorImageInfo Texture::GetDescriptorInfo()
{
    // TODO: Allow doing this explicitly instead, as we can't read
    // the intent behind calling `Get` this can lead to
    // unexpected results
    WaitTransfer();
    return VkDescriptorImageInfo 
    {
        .sampler = m_Sampler,
        .imageView = m_ImageView,
        .imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
}

std::optional<ImageMemoryBarrier> Texture::TakePendingAcquire()
{
    // If the transfer hasn't completed, we'll be giving a barrier that may be set
    // prior to the transfer (and its subsequent release barrier)
    WaitTransfer();
    return std::move(m_PendingAcquireBarrier);
}

void Texture::CreateTextureSampler(VkDevice device, const PhysicalDevice& physicalDevice)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VkFilter::VK_FILTER_LINEAR;
    samplerInfo.minFilter = VkFilter::VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = physicalDevice.GetProperties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VkCompareOp::VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create sampler for texture");
    }
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

void Texture::TransitionLayout(VkImageLayout from, VkImageLayout to, CommandBuffer &commandBuffer, std::optional<Queue> destinationQueue)
{
    // Only handle transfer requests for post-upload QOT requests
    // TODO: Properly handle other QOTs as well?
    assert(destinationQueue.has_value() ^ !(from == VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                                            to == VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    auto shouldTransfer = destinationQueue.has_value() && destinationQueue->RequiresTransfer(commandBuffer.GetQueue());
    
    auto queueSpecifier = shouldTransfer ? QueueSpecifier
    {
        .SourceQueue = commandBuffer.GetQueue(), .DestionationQueue = *destinationQueue
    } : std::optional<QueueSpecifier>(std::nullopt);

    auto barrier = ImageMemoryBarrier{*this,
        queueSpecifier,
        0, 0, from, to, 0, 0};
    if (from == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED && to == VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.Barrier.SourceAccessMask = 0;   
        barrier.Barrier.DestinationAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.SourceStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.DestinationStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (from == VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             to == VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.Barrier.SourceAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        // We'll split the barrier if we transfer (and VK_ACCESS_SHADER_READ_BIT
        // won't mean anything when performed on a dedicated transfer queue).
        // Same for the pipeline stage.
        // TODO: Allow for more granularity than general shader read?
        barrier.Barrier.DestinationAccessMask = shouldTransfer ? 0 : VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
        barrier.SourceStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
        // TODO: Be more general or allow specifying it?
        barrier.DestinationStageMask = shouldTransfer ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : 
            // TODO: Allow reads in vertex/other shader stages?
            VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("Combination not supported");
    }

    commandBuffer.InsertBarrier(barrier);

    if (shouldTransfer)
    {
        ImageMemoryBarrier acquireBarrier
        {
            (*this), 
            queueSpecifier, 
            // TODO: Allow for more granularity than general shader read?
            0, VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT,
            // See vulkan spec 1.3 - 6.6.1: we need to re-specify the 
            // layout transition parameters here, even though the transition
            // already occurred during the release
            from,
            to,
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            // TODO: Allow reads in vertex/other shader stages?
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                       VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		};
		m_PendingAcquireBarrier.emplace(acquireBarrier);
    }
}

void Texture::WaitTransfer()
{
    if (m_PendingTransferFence)
    {
        m_PendingTransferFence->WaitAndReset();   
		m_PendingTransferFence = nullptr;
	}
}


#include <backend/Texture.h>

#include <stdexcept>

#include <backend/PhysicalDevice.h>

constexpr std::array<VkFormat, 3> g_DepthFormats = {VkFormat::VK_FORMAT_D32_SFLOAT, VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                 VkFormat::VK_FORMAT_D24_UNORM_S8_UINT};


bool HasStencilComponent(VkFormat format)
{
    return format == VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageAspectFlags GetMatchingAspectFlags(VkFormat format)
{
    if (format == VkFormat::VK_FORMAT_R8G8B8A8_SRGB)
    {
        return VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (std::find(g_DepthFormats.begin(), g_DepthFormats.end(), format) != g_DepthFormats.end())
    {
        if (HasStencilComponent(format))
        {
            return VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
        {
            return VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
        }
    }
    throw std::runtime_error("Unsupported format");
}

VkImageUsageFlags GetMatchingUsageFlags(VkFormat format)
{
    if (format == VkFormat::VK_FORMAT_R8G8B8A8_SRGB)
    {
        // TODO: Don't let this decide that the usage is transfer
        return VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    else if (std::find(g_DepthFormats.begin(), g_DepthFormats.end(), format) != g_DepthFormats.end())
    {
        return VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    throw std::runtime_error("Unsupported format");
}

VkImageSubresourceRange GetSubResourceRange(VkFormat format)
{
    return VkImageSubresourceRange{
        .aspectMask = GetMatchingAspectFlags(format),
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
}

VkDeviceSize Texture2DCreateInfo::BufferSize() const
{
	// TODO: Fix size for varying channels
    return Width * Height * 4;
}

Texture::Texture(VkDevice device, const PhysicalDevice& physicalDevice, const TextureCreateInfo &createInfo) : 
    m_Device(device), m_Width(createInfo.Width), m_Height(createInfo.Height), m_Format(createInfo.Format)
{
    VkImageCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
    vkCreateInfo.extent = {createInfo.Width, createInfo.Height, 1};
    vkCreateInfo.mipLevels = 1;
    vkCreateInfo.arrayLayers = 1;

    vkCreateInfo.format = createInfo.Format;
    vkCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
    vkCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateInfo.usage = GetMatchingUsageFlags(createInfo.Format);
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
    BindMemory();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = createInfo.Format;
    viewInfo.subresourceRange = GetSubResourceRange(createInfo.Format);

    if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture image view");
    }
}

Texture::Texture(Texture &&other)
{
    *this = std::move(other);
}

Texture &Texture::operator=(Texture &&other)
{
    if (other.m_Image == m_Image)
    {
        return *this;
    }
    
    Destroy();
    m_Image = std::exchange(other.m_Image, VK_NULL_HANDLE);
    m_Memory = std::exchange(other.m_Memory, VK_NULL_HANDLE);
    m_ImageView = std::exchange(other.m_ImageView, VK_NULL_HANDLE);
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Format = other.m_Format;
    return *this;
}

Texture::~Texture()
{
    Destroy();
}

void Texture::Destroy()
{
    if (m_Image != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
        vkDestroyImage(m_Device, m_Image, nullptr);
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }
}

VkImage Texture::Get() const
{
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

VkImageView Texture::GetView() const
{
    return m_ImageView;
}

std::optional<ImageMemoryBarrier> Texture::TransitionLayout(VkImageLayout from, VkImageLayout to, CommandBuffer& commandBuffer, std::optional<Queue> destinationQueue)
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
        0, 0, from, to, 
        GetSubResourceRange(m_Format),
        0, 0};
    if (from == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED && to == VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.Barrier.SourceAccessMask = 0;   
        barrier.Barrier.DestinationAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.SourceStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.DestinationStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    else if (from == VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED && to == VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.Barrier.SourceAccessMask = 0;
        barrier.Barrier.DestinationAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                                VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT |
                                                VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;

        barrier.SourceStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.DestinationStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
            GetSubResourceRange(m_Format),
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            // TODO: Allow reads in vertex/other shader stages?
			VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                       VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		};
		return acquireBarrier;
    }
    else
    {
        return std::nullopt;
    }
}

VkFormat Texture::GetFormat() const
{
    return m_Format;
}

void Texture::BindMemory()
{
    vkBindImageMemory(m_Device, m_Image, m_Memory, 0);
}

VkDescriptorImageInfo Texture::GetDescriptorInfo() const
{
    return VkDescriptorImageInfo 
    {
        // Overriden by the caller side if they have a sampler
        .sampler = VK_NULL_HANDLE,
        .imageView = m_ImageView,
        .imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
}

DepthAttachment::DepthAttachment(VkDevice device, const PhysicalDevice &physicalDevice,
                                 const DepthAttachmentCreateInfo &createInfo, CommandBuffer &graphicsCommandBuffer)
    : m_Texture(device, physicalDevice,
                TextureCreateInfo{createInfo.Width, createInfo.Height, DetermineDepthFormat(physicalDevice)})
{
    graphicsCommandBuffer.BeginSingleTake();
    
    m_Texture.TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, graphicsCommandBuffer,
                               {});
    m_PendingTransferFence = &graphicsCommandBuffer.End();
}

VkAttachmentDescription DepthAttachment::GetAttachmentDescription() const
{
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = m_Texture.GetFormat();
	depthAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return depthAttachment;
}

VkImageView DepthAttachment::GetView()
{
    if (m_PendingTransferFence)
    {
        m_PendingTransferFence->WaitAndReset();
        m_PendingTransferFence = nullptr;
    }
    return m_Texture.GetView();
}

VkFormat DepthAttachment::DetermineDepthFormat(const PhysicalDevice &physicalDevice)
{
    VkFormat format = physicalDevice.FindFirstSupportedFormat(
        {VkFormat::VK_FORMAT_D32_SFLOAT, VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT, VkFormat::VK_FORMAT_D24_UNORM_S8_UINT},
        VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
        VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    return format;
}


Texture2D::Texture2D(VkDevice device, const PhysicalDevice &physicalDevice, const Texture2DCreateInfo &textureCreateInfo, CommandBuffer& transferCommandBuffer,
    Queue destinationQueue) : 
    m_Device(device),
    m_StagingBuffer(CreateStagingBuffer(textureCreateInfo.BufferSize(), physicalDevice, device)),
    m_Texture(Texture(device, physicalDevice, TextureCreateInfo{ textureCreateInfo.Width, textureCreateInfo.Height, VkFormat::VK_FORMAT_R8G8B8A8_SRGB })),
    m_Sampler(CreateTextureSampler(device, physicalDevice))
{
    m_StagingBuffer.UploadData(textureCreateInfo.Data);

    transferCommandBuffer.BeginSingleTake();
    m_Texture.TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     transferCommandBuffer,
                     std::nullopt);
    transferCommandBuffer.CopyBufferToImage(m_StagingBuffer, *this);

    m_PendingAcquireBarrier = m_Texture.TransitionLayout(VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, transferCommandBuffer, destinationQueue);
    m_PendingTransferFence = &transferCommandBuffer.End();
}

Texture2D::Texture2D(Texture2D && other) : 
    m_Device(other.m_Device), m_StagingBuffer(std::move(other.m_StagingBuffer)), 
    m_Texture(std::move(other.m_Texture)),
    m_PendingAcquireBarrier(std::move(other.m_PendingAcquireBarrier)), 
    m_PendingTransferFence(std::move(other.m_PendingTransferFence)), 
    m_Sampler(std::exchange(other.m_Sampler, VK_NULL_HANDLE))
{  
}

Texture2D &Texture2D::operator=(Texture2D &&other)
{
    m_Device = other.m_Device;
    m_StagingBuffer = std::move(other.m_StagingBuffer);
    m_Texture = std::move(other.m_Texture);
    m_PendingAcquireBarrier = std::move(other.m_PendingAcquireBarrier),
    m_PendingTransferFence = std::move(other.m_PendingTransferFence); 
    m_Sampler = std::exchange(other.m_Sampler, VK_NULL_HANDLE);
    return *this;
}

Texture2D::~Texture2D()
{
    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device, m_Sampler, nullptr);
    }
}

VkImage Texture2D::Get()
{
    // TODO: Allow doing this explicitly instead, as we can't read
    // the intent behind calling `Get` this can lead to
    // unexpected results
    WaitTransfer();
    return m_Texture.Get();
}

uint32_t Texture2D::GetWidth() const
{
    return m_Texture.GetWidth();
}

uint32_t Texture2D::GetHeight() const
{
    return m_Texture.GetHeight();
}

VkDescriptorImageInfo Texture2D::GetDescriptorInfo()
{
    // TODO: Allow doing this explicitly instead, as we can't read
    // the intent behind calling `Get` this can lead to
    // unexpected results
    WaitTransfer();
    auto descriptorInfo = m_Texture.GetDescriptorInfo();
    descriptorInfo.sampler = m_Sampler;
    return descriptorInfo;
}

std::optional<ImageMemoryBarrier> Texture2D::TakePendingAcquire()
{
    // If the transfer hasn't completed, we'll be giving a barrier that may be set
    // prior to the transfer (and its subsequent release barrier)
    WaitTransfer();
    return std::move(m_PendingAcquireBarrier);
}

VkSampler Texture2D::CreateTextureSampler(VkDevice device, const PhysicalDevice &physicalDevice)
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

    VkSampler sampler;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VkResult::VK_SUCCESS)
    {
        throw std::runtime_error("Could not create sampler for texture");
    }
    return sampler;
}

DeviceBuffer Texture2D::CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const
{
    auto createStagingBufferInfo = CreateBufferInfo{size, VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                    false};
    return DeviceBuffer(device, physicalDevice, createStagingBufferInfo);
}

void Texture2D::WaitTransfer()
{
    if (m_PendingTransferFence)
    {
        m_PendingTransferFence->WaitAndReset();   
		m_PendingTransferFence = nullptr;
	}
}

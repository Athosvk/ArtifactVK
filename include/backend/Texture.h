#pragma once
#include <span>
#include <memory>

#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "Fence.h"

class PhysicalDevice;

struct Texture2DCreateInfo
{
    uint32_t Width;
    uint32_t Height;
    std::span<const unsigned char> Data;

    VkDeviceSize BufferSize() const;
};

struct TextureCreateInfo
{
    uint32_t Width;
    uint32_t Height;
    VkFormat Format;
    VkImageUsageFlags Usage;
};

class Texture
{
  public:
    Texture(VkDevice device, const PhysicalDevice& physicalDevice, const TextureCreateInfo& createInfo);
    Texture(const Texture &) = delete;
    Texture(Texture &&other);

    Texture &operator=(const Texture &) = delete;
    Texture &operator=(Texture &&other);
    ~Texture();

    VkImage Get() const;
    VkImageView GetView() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    VkDescriptorImageInfo GetDescriptorInfo() const;
    /// <summary>
    /// Perform a transition layout and optionally perform a QFOT-release, returning the acquire.
    /// </summary>
    /// <param name="from">Source layout</param>
    /// <param name="to">Target layout</param>
    /// <param name="commandBuffer">Command buffer to perform the transition on</param>
    /// <param name="destinationQueue">The queue to transfer the image to, if desired</param>
    /// <returns>The matching acquire operation for the QFOT release, if it was needed for the target queue</returns>
    std::optional<ImageMemoryBarrier> TransitionLayout(VkImageLayout from, VkImageLayout to, CommandBuffer &commandBuffer, std::optional<Queue> destinationQueue);
    VkFormat GetFormat() const;
  private:
    void BindMemory();
    void Destroy();

    VkDevice m_Device;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory;
    VkImageView m_ImageView;

    uint32_t m_Width;
    uint32_t m_Height;
    VkFormat m_Format;
};

struct DepthAttachmentCreateInfo
{
    uint32_t Width;
    uint32_t Height;
};

class DepthAttachment
{
public:
    DepthAttachment(VkDevice device, const PhysicalDevice &physicalDevice, const DepthAttachmentCreateInfo &createInfo,
                    CommandBuffer &graphicsCommandBuffer);

    VkAttachmentDescription GetAttachmentDescription() const;
    VkImageView GetView();

  private:
    VkFormat DetermineDepthFormat(const PhysicalDevice &physicalDevice);

    Texture m_Texture;
    Fence* m_PendingTransferFence = nullptr;
};

class Texture2D
{
public:
    Texture2D(VkDevice device, const PhysicalDevice &physicalDevice, const Texture2DCreateInfo &textureCreateInfo, CommandBuffer& transferCommandBuffer,
        Queue destinationQueue);
    Texture2D(const Texture2D &) = delete;
    Texture2D(Texture2D && other);
    Texture2D& operator=(const Texture2D & other) = delete;
    Texture2D& operator=(Texture2D && other);
    ~Texture2D();

    VkImage Get();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    /// <summary>
    /// Takes the transfer acquire barrier, if there is any, for a previously enqueued release barrier used for uploading data
    /// </summary>
    std::optional<ImageMemoryBarrier> TakePendingAcquire();
    VkDescriptorImageInfo GetDescriptorInfo();
  private:
    VkSampler CreateTextureSampler(VkDevice device, const PhysicalDevice& physicalDevice);
    DeviceBuffer CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const;
    void WaitTransfer();

    VkDevice m_Device;
    DeviceBuffer m_StagingBuffer;
    Texture m_Texture;

    std::optional<ImageMemoryBarrier> m_PendingAcquireBarrier;
    Fence* m_PendingTransferFence = nullptr;
    VkSampler m_Sampler;
};

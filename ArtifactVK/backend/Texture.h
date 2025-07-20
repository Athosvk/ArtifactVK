#pragma once
#include <span>
#include <memory>

#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "Fence.h"

class PhysicalDevice;

struct TextureCreateInfo
{
    uint32_t Width;
    uint32_t Height;
    std::span<const unsigned char> Pixels;

    VkDeviceSize BufferSize() const;
};

class Texture
{
public:
    Texture(VkDevice device, const PhysicalDevice &physicalDevice, const TextureCreateInfo &textureCreateInfo, CommandBuffer& transferCommandBuffer,
        Queue destinationQueue);
    Texture(const Texture &) = delete;
    Texture(Texture && other);
    ~Texture();

    VkImage Get();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    /// <summary>
    /// Takes the transfer acquire barrier, if there is any, for a previously enqueued release barrier used for uploading data
    /// </summary>
    std::optional<ImageMemoryBarrier> TakePendingAcquire();
    VkDescriptorImageInfo GetDescriptorInfo();
  private:
    void CreateTextureSampler(VkDevice device, const PhysicalDevice& physicalDevice);
    DeviceBuffer CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const;
    void TransitionLayout(VkImageLayout from, VkImageLayout to, CommandBuffer &commandBuffer, std::optional<Queue> destinationQueue);
    void WaitTransfer();

    VkDevice m_Device;
    DeviceBuffer m_StagingBuffer;
    VkImage m_Image;
    VkDeviceMemory m_Memory;
    uint32_t m_Width;
    uint32_t m_Height;

    std::optional<ImageMemoryBarrier> m_PendingAcquireBarrier;
    Fence* m_PendingTransferFence;
    VkImageView m_ImageView;
    VkSampler m_Sampler;
};

#pragma once
#include <span>

#include <vulkan/vulkan.h>

#include "Buffer.h"

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

    VkImage Get() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

  private:
    DeviceBuffer CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const;
    void TransitionLayout(VkImageLayout from, VkImageLayout to, CommandBuffer &commandBuffer, Queue destinationQueue);

    VkDevice m_Device;
    DeviceBuffer m_StagingBuffer;
    VkImage m_Image;
    VkDeviceMemory m_Memory;
    uint32_t m_Width;
    uint32_t m_Height;
};

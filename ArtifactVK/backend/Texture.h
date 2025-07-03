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
    Texture(VkDevice device, const PhysicalDevice &physicalDevice, const TextureCreateInfo &textureCreateInfo, CommandBuffer& transferCommandBuffer);
    ~Texture();

private:
    DeviceBuffer CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const;

    VkDevice m_Device;
    DeviceBuffer m_StagingBuffer;
    VkImage m_Image;
    VkDeviceMemory m_Memory;
};

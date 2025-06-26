#pragma once
#include <span>

#include "Buffer.h"

struct TextureCreateInfo
{
    uint32_t Width;
    uint32_t Height;
    std::span<char> Pixels;

    VkDeviceSize BufferSize() const;
};

class Texture
{
public:
    Texture(VkDevice device, const PhysicalDevice &physicalDevice, const TextureCreateInfo &textureCreateDesc);

private:
    DeviceBuffer CreateStagingBuffer(size_t size, const PhysicalDevice &physicalDevice, VkDevice device) const;

    DeviceBuffer m_StagingBuffer;
};

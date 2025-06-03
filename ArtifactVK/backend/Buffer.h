#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class PhysicalDevice;

struct CreateBufferInfo
{
    VkDeviceSize Size;
    VkBufferUsageFlags BufferUsage;
    VkMemoryPropertyFlags MemoryProperties;
    VkSharingMode SharingMode;
};

class DeviceBuffer
{
public:
	DeviceBuffer(VkDevice device, const PhysicalDevice& physicalDevice, CreateBufferInfo bufferInfo);
	DeviceBuffer(const DeviceBuffer &buffer) = delete;
	DeviceBuffer(DeviceBuffer &&buffer);
	~DeviceBuffer();

    VkBuffer Get() const;
    VkDeviceSize GetSize() const;

    template<typename T>
    void UploadData(const std::vector<T> vertexData)
    {
        void *targetBuffer;
        auto bufferSize = vertexData.size() * sizeof(T);
        vkMapMemory(m_Device, m_Memory, 0, bufferSize, 0, &targetBuffer);
        memcpy(targetBuffer, vertexData.data(), bufferSize);
        vkUnmapMemory(m_Device, m_Memory);
    }
private:
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags, const PhysicalDevice& physicalDevice) const;

	VkDevice m_Device;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
    CreateBufferInfo m_CreateInfo;
};

#pragma once
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <backend/Barrier.h>

class UniformBuffer;
class Texture2D;
class DescriptorPool;
class DescriptorSet;
class ExtensionFunctionMapping;

class BindSet
{
  private:
    struct BindEntry
    {
        VkWriteDescriptorSet StagingDescriptorWrite;
        union {
            VkDescriptorBufferInfo BufferInfo;
            VkDescriptorImageInfo ImageInfo;
        } DataInfo;
    };

  public:
    BindSet(const DescriptorSet& descriptorSet, VkDevice device);

    BindSet& BindTexture(Texture2D& texture) &;
    BindSet& BindUniformBuffer(const UniformBuffer& buffer) &;
    [[nodiscard]] BindSet&& BindTexture(Texture2D& texture) &&;
    [[nodiscard]] BindSet&& BindUniformBuffer(const UniformBuffer& buffer) &&;
    void FlushWrites();
    std::vector<ImageMemoryBarrier> TakePendingAcquires();
    const DescriptorSet &GetDescriptorSet() const;
  private:
    void BindTextureInternal(Texture2D &texture);
    void BindUniformBufferInternal(const UniformBuffer &buffer);

    const DescriptorSet& m_DescriptorSet;
    std::vector<BindEntry> m_Entries;

	// Unfortunately, as we're taking pending acquires here,
	// this assumes that the first call to Bind for a specifc resource
	// assumes that that person won't bind _and_ submit the same resource
	// in a different BindSet prior to submitting this `BindSet`.
	// TODO: Use references to the resources instead, so that we can
	// fetch the pending acqquires at the time of invoking the call to bind.
    std::vector<ImageMemoryBarrier> m_PendingAcquires;
    VkDevice m_Device;
};

class DescriptorSetLayout
{
  public:
    DescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayout();
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&& other);

    VkDescriptorSetLayout Get() const;
  private:
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    VkDevice m_Device;

    // For validation only
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings; 
};

class DescriptorSet
{
  public:
    DescriptorSet(const DescriptorSetLayout& layout, VkDevice device, VkDescriptorSet set);

    [[nodiscard]] BindSet BindTexture(Texture2D& texture);
    [[nodiscard]] BindSet BindUniformBuffer(const UniformBuffer& buffer);
    VkDescriptorSet Get() const;
    const DescriptorSetLayout& GetLayout() const;
    void SetName(const std::string &name, const ExtensionFunctionMapping& mapping);
  private:
    const DescriptorSetLayout& m_Layout;
    VkDevice m_Device;
    VkDescriptorSet m_DescriptorSet;
};

class DescriptorSetBuilder
{
  public:
    DescriptorSetBuilder& AddUniformBuffer();
    DescriptorSetBuilder& AddTexture();
    /// <summary>
    /// Builds a descriptor set layout and clears the current builder
    /// </summary>
    /// <param name="device">The device to use for building</param>
    /// <returns>The layout based on the bindings</returns>
    DescriptorSetLayout Build(VkDevice device);
  private:
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
};

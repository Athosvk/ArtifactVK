#pragma once
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

class UniformBuffer;
class Texture;
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

    BindSet& BindTexture(Texture& texture) &;
    BindSet& BindUniformBuffer(const UniformBuffer& buffer) &;
    [[nodiscard]] BindSet&& BindTexture(Texture& texture) &&;
    [[nodiscard]] BindSet&& BindUniformBuffer(const UniformBuffer& buffer) &&;
    void Finish();

  private:
    void BindTextureInternal(Texture &texture);
    void BindUniformBufferInternal(const UniformBuffer &buffer);

    const DescriptorSet& m_DescriptorSet;
    std::vector<BindEntry> m_Entries;
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

    [[nodiscard]] BindSet BindTexture(Texture& texture);
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

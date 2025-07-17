#pragma once
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

class UniformBuffer;
class Texture;
class DescriptorPool;
class DescriptorSet;

class BindSet
{
  public:
    BindSet(const DescriptorSet& descriptorSet, VkDevice device);
    BindSet(const BindSet &) = delete;
    BindSet(BindSet&& other);
    ~BindSet();

    BindSet &operator=(const BindSet &) = delete;
    BindSet &operator=(BindSet && other);

    BindSet &BindTexture(const Texture& texture);
    BindSet &BindUniformBuffer(const UniformBuffer& buffer);
    void Finish();

  private:
    std::reference_wrapper<const DescriptorSet> m_DescriptorSet;
    std::vector<VkWriteDescriptorSet> m_StagingDescriptorSetWrites;
    std::vector<std::unique_ptr<VkDescriptorBufferInfo>> m_BufferInfos;
    VkDevice m_Device;
    bool m_FinishedOrMoved = false;
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

    BindSet BindTexture(const Texture& texture);
    BindSet BindUniformBuffer(const UniformBuffer& buffer);
    VkDescriptorSet Get() const;
    const DescriptorSetLayout& GetLayout() const;
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

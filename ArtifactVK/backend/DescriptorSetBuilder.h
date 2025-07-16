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
    const DescriptorSet &m_DescriptorSet;
    std::vector<VkWriteDescriptorSet> m_StagingDescriptorSetWrites;
    VkDevice m_Device;
    bool m_FinishedOrMoved = false;
};

class DescriptorSet
{
  public:
    DescriptorSet(VkDescriptorSetLayout layout, VkDevice device, VkDescriptorSet set);
    DescriptorSet(const DescriptorSet &) = delete;
    DescriptorSet(DescriptorSet && other);
    ~DescriptorSet();

    BindSet BindTexture(const Texture& texture);
    BindSet BindUniformBuffer(const UniformBuffer& buffer);
    VkDescriptorSet Get() const;
    VkDescriptorSetLayout GetLayout() const;
  private:
    VkDescriptorSetLayout m_Layout;
    VkDevice m_Device;
    VkDescriptorSet m_DescriptorSet;
    // For validation only
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
};

class DescriptorSetBuilder
{
  public:
    DescriptorSetBuilder& AddUniformBuffer();
    DescriptorSetBuilder& AddTexture();
    DescriptorSet Build(DescriptorPool& pool, VkDevice device);
  private:
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
};

#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <optional>

#include "ShaderModule.h"
#include "RenderPass.h"

struct Viewport;
class VulkanDevice;
class UniformBuffer;

struct PipelineCreateInfo
{
    VkGraphicsPipelineCreateInfo CreateInfo;
    std::vector<VkDescriptorSetLayout> Descriptors;
    const RenderPass &RenderPass;
};

class RasterPipeline
{
  public:
    RasterPipeline(VkDevice vulkanDevice, PipelineCreateInfo createInfo);

    RasterPipeline(const RasterPipeline &) = delete;
    RasterPipeline(RasterPipeline &&other);
    ~RasterPipeline();

    RasterPipeline &operator=(const ShaderModule &) = delete;
    RasterPipeline &operator=(ShaderModule &&) = delete;

    void Bind(const VkCommandBuffer &commandBuffer, const Viewport& viewport) const;
  private:
    VkDevice m_VulkanDevice;
    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_Pipeline;
};

struct VertexBindingDescription 
{
    VkVertexInputBindingDescription Description;
    std::array<VkVertexInputAttributeDescription, 2> AttributeDescriptions; 

    VkPipelineVertexInputStateCreateInfo GetVkPipelineInputStateCreateInfo() const;
    static VkPipelineVertexInputStateCreateInfo DefaultPipelineInputStateCreateInfo();
};

class RasterPipelineBuilder
{
  public:
    RasterPipelineBuilder(std::filesystem::path &&vertexShaderPath, std::filesystem::path &&fragmentShaderPath);

    RasterPipelineBuilder& SetVertexBindingDescription(const VertexBindingDescription& vertexBinding);
    RasterPipelineBuilder& AddDescriptorSet(VkDescriptorSetLayout uniformBuffer);
    const std::optional<VertexBindingDescription>& GetVertexBindingDescription() const;
    const std::filesystem::path& GetVertexShaderPath() const;
    const std::filesystem::path& GetFragmentShaderPath() const;
    const std::vector<VkDescriptorSetLayout>& GetDescriptorSets() const;
  private:
    std::filesystem::path m_VertexShaderPath;
    std::filesystem::path m_FragmentShaderPath;
    std::optional<VertexBindingDescription> m_VertexBindingDescription;
    std::vector<VkDescriptorSetLayout> m_DescriptorSets;
};

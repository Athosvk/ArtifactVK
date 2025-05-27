#pragma once
#include <vulkan/vulkan.h>

#include <array>
#include <optional>

#include "ShaderModule.h"
#include "RenderPass.h"

struct Viewport;
class LogicalVulkanDevice;

class RasterPipeline
{
  public:
    RasterPipeline(VkDevice vulkanDevice, VkGraphicsPipelineCreateInfo createInfo, const RenderPass& renderPass);

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
    const std::optional<VertexBindingDescription>& GetVertexBindingDescription() const;
    const std::filesystem::path& GetVertexShaderPath() const;
    const std::filesystem::path& GetFragmentShaderPath() const;
  private:
    std::filesystem::path m_VertexShaderPath;
    std::filesystem::path m_FragmentShaderPath;
    std::optional<VertexBindingDescription> m_VertexBindingDescription;
};

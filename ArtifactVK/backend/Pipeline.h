#pragma once
#include "ShaderModule.h"
#include "RenderPass.h"
#include <vulkan/vulkan.h>

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

class RasterPipelineBuilder
{
  public:
    RasterPipelineBuilder(std::filesystem::path &&vertexShaderPath, std::filesystem::path &&fragmentShaderPath);

    const std::filesystem::path& GetVertexShaderPath() const;
    const std::filesystem::path& GetFragmentShaderPath() const;
    bool RenderToSwapchain() const;
  private:
    std::filesystem::path m_VertexShaderPath;
    std::filesystem::path m_FragmentShaderPath;
};

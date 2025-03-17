#pragma once
#include "ShaderModule.h"
#include <vulkan/vulkan.h>

class LogicalVulkanDevice;


class RasterPipeline
{
  public:
    explicit RasterPipeline(VkDevice &vulkanDevice);

    RasterPipeline(const RasterPipeline &) = delete;
    RasterPipeline(RasterPipeline &&other);
    ~RasterPipeline();

    RasterPipeline &operator=(const ShaderModule &) = delete;
    RasterPipeline &operator=(ShaderModule &&) = delete;

  private:
    VkDevice &m_VulkanDevice;
    VkPipelineLayout m_PipelineLayout;
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

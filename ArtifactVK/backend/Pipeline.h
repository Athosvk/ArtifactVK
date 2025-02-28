#pragma once
#include "ShaderModule.h"

class LogicalVulkanDevice;


class RasterPipeline
{
};

class RasterPipelineBuilder
{
  public:
    RasterPipelineBuilder(std::filesystem::path &&vertexShaderPath, std::filesystem::path &&fragmentShaderPath);

    const std::filesystem::path& GetVertexShaderPath() const;
    const std::filesystem::path& GetFragmentShaderPath() const;

  private:
    std::filesystem::path m_VertexShaderPath;
    std::filesystem::path m_FragmentShaderPath;
};

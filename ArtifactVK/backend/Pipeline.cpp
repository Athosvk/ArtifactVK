#include "Pipeline.h"
#include "VulkanDevice.h"

RasterPipelineBuilder::RasterPipelineBuilder(std::filesystem::path&& vertexShaderPath,
                                             std::filesystem::path&& fragmentShaderPath)
    : m_VertexShaderPath(vertexShaderPath), m_FragmentShaderPath(fragmentShaderPath)
{
}

const std::filesystem::path &RasterPipelineBuilder::GetVertexShaderPath() const
{
    return m_VertexShaderPath;
}

const std::filesystem::path &RasterPipelineBuilder::GetFragmentShaderPath() const
{
    return m_FragmentShaderPath;
}


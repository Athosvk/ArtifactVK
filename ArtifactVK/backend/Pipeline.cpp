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

bool RasterPipelineBuilder::RenderToSwapchain() const
{
    // TODO: Change once we can specify different framebuffers to render the pipeline to
    return true;
}

RasterPipeline::RasterPipeline(VkDevice &vulkanDevice)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout))
    {
        throw std::runtime_error("Could not create pipeline layout");
    }
}

RasterPipeline::RasterPipeline(RasterPipeline &&other) : 
    m_VulkanDevice(other.m_VulkanDevice),
    m_PipelineLayout(std::exchange(other.m_PipelineLayout, VK_NULL_HANDLE))
{
    
}

RasterPipeline::~RasterPipeline()
{
    vkDestroyPipelineLayout(m_VulkanDevice, m_PipelineLayout, nullptr);
}

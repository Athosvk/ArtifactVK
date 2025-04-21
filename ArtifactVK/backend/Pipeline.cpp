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

RasterPipeline::RasterPipeline(VkDevice vulkanDevice, VkGraphicsPipelineCreateInfo createInfo, const RenderPass& renderPass)
    : m_VulkanDevice(vulkanDevice) 
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create pipeline layout");
    }
    createInfo.layout = m_PipelineLayout;
    createInfo.renderPass = renderPass.Get();
    createInfo.subpass = 0;

    // Unused
    createInfo.basePipelineHandle = VK_NULL_HANDLE;
    createInfo.basePipelineIndex = -1;
    if (vkCreateGraphicsPipelines(vulkanDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline))
    {
        throw std::runtime_error("Could not create pipeline");
    }
}

RasterPipeline::RasterPipeline(RasterPipeline &&other) : 
    m_VulkanDevice(other.m_VulkanDevice),
    m_PipelineLayout(std::exchange(other.m_PipelineLayout, VK_NULL_HANDLE)),
    m_Pipeline(std::exchange(other.m_Pipeline, VK_NULL_HANDLE))
{
    // TODO: Somehow ensure that this signals the render pass is still bound by this (?)
}

RasterPipeline::~RasterPipeline()
{
    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_VulkanDevice, m_Pipeline, nullptr);
        vkDestroyPipelineLayout(m_VulkanDevice, m_PipelineLayout, nullptr);
    }
}

void RasterPipeline::Bind(const VkCommandBuffer &commandBuffer, const Viewport& viewport) const
{
    vkCmdBindPipeline(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport.Viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &viewport.Scissor);

}

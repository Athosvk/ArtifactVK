#include "Pipeline.h"
#include "VulkanDevice.h"

RasterPipelineBuilder::RasterPipelineBuilder(std::filesystem::path&& vertexShaderPath,
                                             std::filesystem::path&& fragmentShaderPath)
    : m_VertexShaderPath(vertexShaderPath), m_FragmentShaderPath(fragmentShaderPath)
{
}

RasterPipelineBuilder & RasterPipelineBuilder::SetVertexBindingDescription(const VertexBindingDescription & vertexBinding)
{
    m_VertexBindingDescription.emplace(vertexBinding);
    return *this;
}

RasterPipelineBuilder &RasterPipelineBuilder::AddDescriptorSet(VkDescriptorSetLayout descriptor)
{
    m_DescriptorSets.push_back(descriptor);
    return *this;
}

const std::optional<VertexBindingDescription>& RasterPipelineBuilder::GetVertexBindingDescription() const
{
    return m_VertexBindingDescription;
}

const std::filesystem::path &RasterPipelineBuilder::GetVertexShaderPath() const
{
    return m_VertexShaderPath;
}

const std::filesystem::path &RasterPipelineBuilder::GetFragmentShaderPath() const
{
    return m_FragmentShaderPath;
}

const std::vector<VkDescriptorSetLayout>& RasterPipelineBuilder::GetDescriptorSets() const
{
    return m_DescriptorSets;
}

RasterPipeline::RasterPipeline(VkDevice vulkanDevice, PipelineCreateInfo createInfo)
    : m_VulkanDevice(vulkanDevice) 
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = createInfo.Descriptors.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(vulkanDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create pipeline layout");
    }
    VkGraphicsPipelineCreateInfo vkCreateInfo = std::move(createInfo.CreateInfo);
    vkCreateInfo.layout = m_PipelineLayout;
    vkCreateInfo.renderPass = createInfo.RenderPass.Get();
    vkCreateInfo.subpass = 0;

    // Unused
    vkCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    vkCreateInfo.basePipelineIndex = -1;
    if (vkCreateGraphicsPipelines(vulkanDevice, VK_NULL_HANDLE, 1, &vkCreateInfo, nullptr, &m_Pipeline))
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

VkPipelineVertexInputStateCreateInfo VertexBindingDescription::GetVkPipelineInputStateCreateInfo() const
{
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &Description;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();
    return vertexInputCreateInfo;
}

VkPipelineVertexInputStateCreateInfo VertexBindingDescription::DefaultPipelineInputStateCreateInfo()
{
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
    return vertexInputCreateInfo;
}

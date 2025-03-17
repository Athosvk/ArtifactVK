#include "RenderPass.h"

#include <stdexcept>

RenderPass::RenderPass(VkDevice device, RenderPassCreateInfo &&createInfo) : m_Device(device)
{
    VkAttachmentReference swapchainAttachmentRef{};
    swapchainAttachmentRef.attachment = 0;
    swapchainAttachmentRef.layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &swapchainAttachmentRef;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &createInfo.attachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_Device, &renderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create render pass");
    }
}

RenderPass::~RenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    }
}

RenderPass::RenderPass(RenderPass &&other) : m_Device(other.m_Device), m_RenderPass(std::exchange(other.m_RenderPass, VK_NULL_HANDLE))
{
}

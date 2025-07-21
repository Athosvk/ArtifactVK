#include "Framebuffer.h"
#include "RenderPass.h"

#include <stdexcept>

Framebuffer::Framebuffer(VkDevice device, const FramebufferCreateInfo &createInfo)
    : m_Device(device), m_OriginalCreateInfo(createInfo)
{
	VkImageView swapchainColorAttachment[] = {createInfo.ColorOutputView, createInfo.DepthAttachmentView};
	VkFramebufferCreateInfo framebufferCreateInfo{}; 
	framebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = createInfo.RenderPass.Get();
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = swapchainColorAttachment;
	framebufferCreateInfo.width = static_cast<uint32_t>(createInfo.Viewport.Viewport.width);
	framebufferCreateInfo.height = static_cast<uint32_t>(createInfo.Viewport.Viewport.height);
	framebufferCreateInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create swapchain framebuffers");
	}
}

Framebuffer::Framebuffer(Framebuffer &&other) : 
	m_Framebuffer(std::exchange(other.m_Framebuffer, VK_NULL_HANDLE)), m_Device(other.m_Device),
    m_OriginalCreateInfo(other.m_OriginalCreateInfo)
{
}

Framebuffer::~Framebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
	}
}

VkFramebuffer Framebuffer::Get() const
{
    return m_Framebuffer;
}

Viewport Framebuffer::GetViewport() const
{
    return m_OriginalCreateInfo.Viewport;
}

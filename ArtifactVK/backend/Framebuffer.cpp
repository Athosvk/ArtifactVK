#include "Framebuffer.h"
#include "RenderPass.h"

#include <stdexcept>

Framebuffer::Framebuffer(VkDevice device, const FramebufferCreateInfo& createInfo) : m_Framebuffer()
{
	VkImageView swapchainColorAttachment[] = {createInfo.ImageView};
	VkFramebufferCreateInfo framebufferCreateInfo{}; 
	framebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = createInfo.RenderPass.Get();
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = swapchainColorAttachment;
	framebufferCreateInfo.width = createInfo.Extents.width;
	framebufferCreateInfo.height = createInfo.Extents.height;
	framebufferCreateInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create swapchain framebuffers");
	}
}

Framebuffer::Framebuffer(Framebuffer &&other) : 
	m_Framebuffer(std::exchange(other.m_Framebuffer, VK_NULL_HANDLE)), m_Device(other.m_Device)
{
}

Framebuffer::~Framebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
	}
}

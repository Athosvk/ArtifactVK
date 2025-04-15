#pragma once
#include <vulkan/vulkan.h>

class RenderPass;

struct FramebufferCreateInfo
{
    const RenderPass &RenderPass;
    VkImageView ImageView;
    VkExtent2D Extents;
};

class Framebuffer
{
  public:
    Framebuffer(VkDevice device, const FramebufferCreateInfo& framebufferCreateInfo);
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer(Framebuffer &&other);
    ~Framebuffer();


  private:
    VkFramebuffer m_Framebuffer;
    VkDevice m_Device;
};

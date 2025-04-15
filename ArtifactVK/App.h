#pragma once
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"
#include "backend/Pipeline.h"
#include "backend/RenderPass.h"

class CommandBufferPool;

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    RasterPipeline LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const;

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    RenderPass m_MainPass;
    RasterPipeline m_RenderFullscreen;
    std::span<Framebuffer> m_SwapchainFramebuffers;
    CommandBufferPool &m_GraphicsCommandBufferPool;
};

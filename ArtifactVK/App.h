#pragma once
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"
#include "backend/Pipeline.h"
#include "backend/RenderPass.h"
#include "backend/Swapchain.h"

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    RasterPipeline LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const;
    void RecordCommandBuffer(uint32_t swapchainImageIndex);

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    RenderPass m_MainPass;
    RasterPipeline m_RenderFullscreen;
    const SwapchainFramebuffer& m_SwapchainFramebuffers;
    CommandBuffer &m_GraphicsCommandBuffer;
    Semaphore &m_ImageAvailable;
    Semaphore &m_RenderFinished;
    Fence &m_CommandBufferInFlightFence;
};

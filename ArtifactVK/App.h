#pragma once
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <functional>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"
#include "backend/Pipeline.h"
#include "backend/RenderPass.h"
#include "backend/Swapchain.h"

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    RasterPipeline LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const;
    void RecordCommandBuffer();

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    RenderPass m_MainPass;
    RasterPipeline m_RenderFullscreen;
    const SwapchainFramebuffer& m_SwapchainFramebuffers;
    std::vector<std::reference_wrapper<CommandBuffer>> m_GraphicsCommandBuffers;
    std::vector<std::reference_wrapper<Semaphore>> m_ImageAvailable;
    std::vector<std::reference_wrapper<Semaphore>> m_RenderFinished;
    uint32_t m_CurrentFrameIndex = 0;
    Swapchain &m_Swapchain;
};

#pragma once
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"
#include "backend/Pipeline.h"
#include "backend/RenderPass.h"

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    RasterPipeline LoadShaderPipeline(LogicalVulkanDevice& vulkanDevice) const;

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    RasterPipeline m_RenderFullscreen;
    RenderPass m_RenderPass;
};

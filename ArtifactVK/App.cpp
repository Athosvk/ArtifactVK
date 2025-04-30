#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "backend/ShaderModule.h"

const InstanceCreateInfo DefaultCreateInfo()
{
    InstanceCreateInfo createInfo;
    createInfo.Name = "ArtifactVK";
    createInfo.ValidationLayers =
        std::vector<ValidationLayer>{ValidationLayer{EValidationLayer::KhronosValidation, false}};
    createInfo.RequiredExtensions = std::vector<EDeviceExtension>{EDeviceExtension::VkSwapchain};
    return createInfo;
}

App::App()
    : m_Window(WindowCreateInfo{800, 600, "ArtifactVK"}),
      m_VulkanInstance(m_Window.CreateVulkanInstance(DefaultCreateInfo())),
      m_MainPass(m_VulkanInstance.GetActiveDevice().CreateRenderPass()),
      m_RenderFullscreen(LoadShaderPipeline(m_VulkanInstance.GetActiveDevice(), m_MainPass)),
      m_SwapchainFramebuffers(m_VulkanInstance.GetActiveDevice().CreateSwapchainFramebuffers(m_MainPass)),
      m_GraphicsCommandBuffer(m_VulkanInstance.GetActiveDevice().CreateGraphicsCommandBufferPool().CreateCommandBuffer()),
      m_ImageAvailable(m_VulkanInstance.GetActiveDevice().CreateSemaphore()),
      m_RenderFinished(m_VulkanInstance.GetActiveDevice().CreateSemaphore()),
      m_CommandBufferInFlightFence(m_VulkanInstance.GetActiveDevice().CreateFence())
{
    
}

App::~App()
{
    glfwTerminate();
}

void App::RunRenderLoop()
{
    while (!m_Window.ShouldClose())
    {
        m_Window.PollEvents();
        RecordCommandBuffer(0);
    }
}

RasterPipeline App::LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const
{
    return vulkanDevice.CreateRasterPipeline(
        RasterPipelineBuilder("spirv/triangle.vert.spv", "spirv/triangle.frag.spv"), renderPass);
}

void App::RecordCommandBuffer(uint32_t swapchainImageIndex)
{
    // TODO?: The tutorial says to do this and start the fence signaled, but we could just...
    // not do that and wait at the end of a frame (this seems much more sane). Uncomment
    // this if it doesn't work.
    // m_CommandBufferInFlightFence.Wait();
    m_GraphicsCommandBuffer.Begin();
    m_GraphicsCommandBuffer.Draw(m_SwapchainFramebuffers.GetCurrent(), m_MainPass, m_RenderFullscreen);
    m_GraphicsCommandBuffer.End();
    m_CommandBufferInFlightFence.Wait();
}

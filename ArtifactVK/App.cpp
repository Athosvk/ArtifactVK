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
      m_GraphicsCommandBuffers(m_VulkanInstance.GetActiveDevice().CreateGraphicsCommandBufferPool().CreateCommandBuffers(2)),
      m_ImageAvailable(m_VulkanInstance.GetActiveDevice().CreateDeviceSemaphore()),
      m_RenderFinished(m_VulkanInstance.GetActiveDevice().CreateDeviceSemaphore()),
      m_Swapchain(m_VulkanInstance.GetActiveDevice().GetSwapchain())
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
        std::cout << "\nRendering frame " << m_CurrentFrameIndex << "\n"; 
        m_Window.PollEvents();
        RecordCommandBuffer();
        m_CurrentFrameIndex += 1;
    }
}

RasterPipeline App::LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const
{
    return vulkanDevice.CreateRasterPipeline(
        RasterPipelineBuilder("spirv/triangle.vert.spv", "spirv/triangle.frag.spv"), renderPass);
}

void App::RecordCommandBuffer()
{
    // TODO?: The tutorial says to do this and start the fence signaled, but we could just...
    // not do that and wait at the end of a frame (this seems much more sane). Uncomment
    // this if it doesn't work.
    // m_CommandBufferInFlightFence.Wait();
    m_Swapchain.AcquireNext(m_ImageAvailable);
    m_GraphicsCommandBuffer.Begin();
    m_GraphicsCommandBuffer.Draw(m_SwapchainFramebuffers.GetCurrent(), m_MainPass, m_RenderFullscreen);
    Fence& inFlight = m_GraphicsCommandBuffer.End(std::span{ &m_ImageAvailable, 1 }, std::span{ &m_RenderFinished, 1 }, 
        m_VulkanInstance.GetActiveDevice().GetGraphicsQueue());
    
    m_Swapchain.Present(std::span{&m_ImageAvailable, 1});
    inFlight.WaitAndReset();
}

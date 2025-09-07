#include "ComputeSample.h"

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

InstanceCreateInfo DefaultCreateInfo()
{
    InstanceCreateInfo createInfo;
    createInfo.Name = "ArtifactVK";
    createInfo.ValidationLayers =
        std::vector<ValidationLayer>{ValidationLayer{EValidationLayer::KhronosValidation, false}};
    createInfo.RequiredExtensions = std::vector<EDeviceExtension>{EDeviceExtension::Swapchain};
    return createInfo;
}

ComputeSample::ComputeSample() : 
    m_Window(WindowCreateInfo{800, 600, "ComputeSample"}), 
    m_VulkanInstance(m_Window.CreateVulkanInstance(DefaultCreateInfo())),
    m_DepthAttachment(m_VulkanInstance.GetActiveDevice().CreateSwapchainDepthAttachment()), 
    m_MainPass(m_VulkanInstance.GetActiveDevice().CreateRenderPass(m_DepthAttachment)), 
    m_SwapchainFramebuffers(m_VulkanInstance.GetActiveDevice().CreateSwapchainFramebuffers(m_MainPass, &m_DepthAttachment)),
      m_DescriptorSetLayout(BuildDescriptorSetLayout(m_VulkanInstance.GetActiveDevice())), 
    m_PerFrameState(CreatePerFrameState(m_VulkanInstance.GetActiveDevice())),
    m_RenderFullscreen(LoadShaderPipeline(m_VulkanInstance.GetActiveDevice(), m_MainPass)), 
    m_Swapchain(m_VulkanInstance.GetActiveDevice().GetSwapchain())
{
}

ComputeSample::~ComputeSample()
{
    for (auto &perFrameState : m_PerFrameState)
    {
        perFrameState.CommandBuffer.WaitFence();
    }
    glfwTerminate();
}

void ComputeSample::RunRenderLoop()
{
    while (!m_Window.ShouldClose())
    {
        if (!m_Window.IsMinimized())
        {
            auto resizeEvent = m_Window.PollEvents();
            if (resizeEvent.has_value() && !m_Window.IsMinimized())
            {
                m_VulkanInstance.GetActiveDevice().HandleResizeEvent(*resizeEvent);
            }
            
            if (m_Window.IsMinimized())
            {
                m_Window.WaitForRender();
            }
            else
            {
				RecordFrame(m_PerFrameState[m_CurrentFrameIndex % MAX_FRAMES_IN_FLIGHT]);
				m_CurrentFrameIndex += 1;
            }

        }
    }
}

DepthAttachment &ComputeSample::CreateSwapchainDepthAttachment()
{
    return m_VulkanInstance.GetActiveDevice().CreateSwapchainDepthAttachment();
}

UniformConstants ComputeSample::GetUniforms()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float secondsElapsed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformConstants constants;
    constants.model = glm::rotate(glm::mat4(1.0f), secondsElapsed * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    constants.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    
    constants.projection = glm::perspective(
        glm::radians(45.0f), m_VulkanInstance.GetActiveDevice().GetSwapchain().GetViewportDescription().AspectRatio(),
        0.1f, 10.0f);
    constants.projection[1][1] *= -1;
    return constants;
}

RasterPipeline ComputeSample::LoadShaderPipeline(VulkanDevice &vulkanDevice, const RenderPass &renderPass) const
{
    auto builder = RasterPipelineBuilder("shaders/triangle.vert.spv", "shaders/triangle.frag.spv");
    builder.SetVertexBindingDescription(Vertex::GetVertexBindingDescription());

    // TODO: Have nicer outer bindings for it (i.e. less directly translated from Vulkan)_
    builder.SetDescriptorSetLayout(m_DescriptorSetLayout);
    return vulkanDevice.CreateRasterPipeline(std::move(builder), renderPass);
}

void ComputeSample::RecordFrame(PerFrameState& state)
{
    auto &activeDevice = m_VulkanInstance.GetActiveDevice();
    activeDevice.AcquireNext(state.ImageAvailable);
    // TODO: Can probably be moved to CommandBuffer->Begin()
    state.CommandBuffer.WaitFence();

    auto previousResults = state.TimerPool.Resolve();
    std::chrono::duration<double, std::milli> millis = previousResults.Timings["Frame Total"];
    m_Window.SetTitle(std::format("GPU: {:.5f} ms", millis.count()));
    state.CommandBuffer.Begin();

	// TODO: Shouldn't be the user's burden
	state.CommandBuffer.ResetTimerPool(state.TimerPool);
    {
        state.TimerPool.BeginScope(state.CommandBuffer.Get(), "Frame Total");

        auto uniforms = GetUniforms();
        state.UniformBuffer.UploadData(GetUniforms());
        auto bindSet = state.DescriptorSet.BindUniformBuffer(state.UniformBuffer);
        state.TimerPool.BeginScope(state.CommandBuffer.Get(), "Draw");
        //state.CommandBuffer.Draw(m_SwapchainFramebuffers.GetCurrent(), m_MainPass, m_RenderFullscreen, m_VertexBuffer,
         //                        std::move(bindSet));
    }
    state.CommandBuffer.End(std::span{ &state.ImageAvailable, 1 }, std::span{ &state.RenderFinished, 1 });
    
    activeDevice.Present(std::span{&state.RenderFinished, 1});
}

std::vector<std::reference_wrapper<Semaphore>> ComputeSample::CreateSemaphorePerInFlightFrame()
{
    std::vector<std::reference_wrapper<Semaphore>> semaphores;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        semaphores.emplace_back(m_VulkanInstance.GetActiveDevice().CreateDeviceSemaphore());
    }
    return semaphores;
}

std::vector<PerFrameState> ComputeSample::CreatePerFrameState(VulkanDevice &vulkanDevice)
{
    std::vector<PerFrameState> perFrameState;
    auto commandBuffers = vulkanDevice.GetGraphicsCommandBufferPool().CreateCommandBuffers(
        MAX_FRAMES_IN_FLIGHT, m_VulkanInstance.GetActiveDevice().GetGraphicsQueue());

    perFrameState.reserve(MAX_FRAMES_IN_FLIGHT);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto &uniformBuffer = vulkanDevice.CreateUniformBuffer<UniformConstants>();
        auto descriptorSet = vulkanDevice.CreateDescriptorSet(m_DescriptorSetLayout);

        perFrameState.emplace_back(PerFrameState{vulkanDevice.CreateDeviceSemaphore(),
                                                 vulkanDevice.CreateDeviceSemaphore(), commandBuffers[i],
                                                 uniformBuffer,
                                                 descriptorSet, vulkanDevice.CreateTimerPool()
            });
        descriptorSet.SetName("Descriptor Set frame index " + std::to_string(i), m_VulkanInstance.GetExtensionFunctionMapping());
        commandBuffers[i].get().SetName("Graphics CMD frame index " + std::to_string(i), m_VulkanInstance.GetExtensionFunctionMapping());

    }
    return perFrameState;
}

const DescriptorSetLayout& ComputeSample::BuildDescriptorSetLayout(VulkanDevice &vulkanDevice) const
{
    return vulkanDevice.CreateDescriptorSetLayout(DescriptorSetBuilder().AddUniformBuffer().AddTexture());
}


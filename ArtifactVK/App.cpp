#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>


#include "backend/ShaderModule.h"
#include "backend/DebugMarker.h"

const InstanceCreateInfo DefaultCreateInfo()
{
    InstanceCreateInfo createInfo;
    createInfo.Name = "ArtifactVK";
    createInfo.ValidationLayers =
        std::vector<ValidationLayer>{ValidationLayer{EValidationLayer::KhronosValidation, false}};
    createInfo.RequiredExtensions = std::vector<EDeviceExtension>{EDeviceExtension::Swapchain};
    return createInfo;
}

App::App()
    : m_Window(WindowCreateInfo{800, 600, "ArtifactVK"}),
      m_VulkanInstance(m_Window.CreateVulkanInstance(DefaultCreateInfo())),
      m_MainPass(m_VulkanInstance.GetActiveDevice().CreateRenderPass()),
      m_SwapchainFramebuffers(m_VulkanInstance.GetActiveDevice().CreateSwapchainFramebuffers(m_MainPass)),
      m_DescriptorSetLayout(BuildDescriptorSetLayout(m_VulkanInstance.GetActiveDevice())),
      m_PerFrameState(CreatePerFrameState(m_VulkanInstance.GetActiveDevice())),
      m_RenderFullscreen(LoadShaderPipeline(m_VulkanInstance.GetActiveDevice(), m_MainPass)),
      m_Swapchain(m_VulkanInstance.GetActiveDevice().GetSwapchain()),
      m_VertexBuffer(m_VulkanInstance.GetActiveDevice().CreateVertexBuffer(GetVertices())),
      m_IndexBuffer(m_VulkanInstance.GetActiveDevice().CreateIndexBuffer(GetIndices())), 
      m_Texture(LoadImage())
{
}

App::~App()
{
    for (auto &perFrameState : m_PerFrameState)
    {
        perFrameState.CommandBuffer.WaitFence();
    }
    glfwTerminate();
}

void App::RunRenderLoop()
{
    while (!m_Window.ShouldClose())
    {
        if (!m_Window.IsMinimized())
        {
            //std::cout << "\nRendering frame " << m_CurrentFrameIndex << "\n";
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

Texture& App::LoadImage()
{
    Image image("textures/texture.jpg");
    return m_VulkanInstance.GetActiveDevice().CreateTexture(image.GetTextureCreateDesc());
}

UniformConstants App::GetUniforms()
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

RasterPipeline App::LoadShaderPipeline(VulkanDevice &vulkanDevice, const RenderPass &renderPass) const
{
    auto builder = RasterPipelineBuilder("spirv/shaders/triangle.vert.spv", "spirv/shaders/triangle.frag.spv");
    builder.SetVertexBindingDescription(Vertex::GetVertexBindingDescription());

    // TODO: Have nicer outer bindings for it (i.e. less directly translated from Vulkan)_
    builder.SetDescriptorSetLayout(m_DescriptorSetLayout);
    return vulkanDevice.CreateRasterPipeline(std::move(builder), renderPass);
}

void App::RecordFrame(PerFrameState& state)
{
    m_VulkanInstance.GetActiveDevice().AcquireNext(state.ImageAvailable);
    // TODO: Can probably be moved to CommandBuffer->Begin()
    state.CommandBuffer.WaitFence();
    state.CommandBuffer.Begin();
    auto uniforms = GetUniforms();
    state.UniformBuffer.UploadData(GetUniforms());
    state.DescriptorSet.BindUniformBuffer(state.UniformBuffer).BindTexture(m_Texture).Finish();
    state.CommandBuffer.DrawIndexed(m_SwapchainFramebuffers.GetCurrent(), m_MainPass, m_RenderFullscreen, m_VertexBuffer, m_IndexBuffer, 
        state.DescriptorSet);
    state.CommandBuffer.End(std::span{ &state.ImageAvailable, 1 }, std::span{ &state.RenderFinished, 1 });
    
    m_VulkanInstance.GetActiveDevice().Present(std::span{&state.RenderFinished, 1});
}

std::vector<std::reference_wrapper<Semaphore>> App::CreateSemaphorePerInFlightFrame()
{
    std::vector<std::reference_wrapper<Semaphore>> semaphores;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        semaphores.emplace_back(m_VulkanInstance.GetActiveDevice().CreateDeviceSemaphore());
    }
    return semaphores;
}

std::vector<PerFrameState> App::CreatePerFrameState(VulkanDevice &vulkanDevice)
{
    std::vector<PerFrameState> perFrameState;
    auto commandBuffers = vulkanDevice.CreateGraphicsCommandBufferPool().CreateCommandBuffers(MAX_FRAMES_IN_FLIGHT, m_VulkanInstance.GetActiveDevice().GetGraphicsQueue());

    perFrameState.reserve(MAX_FRAMES_IN_FLIGHT);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto &uniformBuffer = vulkanDevice.CreateUniformBuffer<UniformConstants>();
        auto descriptorSet = vulkanDevice.CreateDescriptorSet(m_DescriptorSetLayout);

        perFrameState.emplace_back(PerFrameState{vulkanDevice.CreateDeviceSemaphore(),
                                                 vulkanDevice.CreateDeviceSemaphore(), commandBuffers[i],
                                                 uniformBuffer,
                                                 descriptorSet
            });
        descriptorSet.SetName("Descriptor Set frame index " + std::to_string(i), m_VulkanInstance.GetExtensionFunctionMapping());
        commandBuffers[i].get().SetName("Graphics CMD frame index " + std::to_string(i), m_VulkanInstance.GetExtensionFunctionMapping());

    }
    return perFrameState;
}

constexpr std::vector<Vertex> App::GetVertices()
{
    return
    {
	    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
}

constexpr std::vector<uint16_t> App::GetIndices()
{
    return {
        0, 1, 2, 2, 3, 0
    };
}

const DescriptorSetLayout& App::BuildDescriptorSetLayout(VulkanDevice &vulkanDevice) const
{
    return vulkanDevice.CreateDescriptorSetLayout(DescriptorSetBuilder().AddUniformBuffer().AddTexture());
}

constexpr VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

constexpr std::array<VkVertexInputAttributeDescription,2> Vertex::GetAttributeDescriptions()
{
    VkVertexInputAttributeDescription positionAttribute;
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, Position);

    VkVertexInputAttributeDescription colorAttribute;
    colorAttribute.binding = 0;
    colorAttribute.location = 1;
    colorAttribute.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, Color);

    return {
        positionAttribute, colorAttribute
    };
}

constexpr VertexBindingDescription Vertex::GetVertexBindingDescription()
{
    return VertexBindingDescription{
        GetBindingDescription(),
        GetAttributeDescriptions()
    };
}

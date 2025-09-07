#include <backend/Window.h>
#include <backend/VulkanInstance.h>
#include <backend/Texture.h>
#include <backend/RenderPass.h>
#include <backend/Swapchain.h>
#include <backend/DescriptorPool.h>
#include <backend/Pipeline.h>
#include <Vertex.h>

struct PerFrameState
{
    Semaphore &ImageAvailable;
    Semaphore &RenderFinished;
    CommandBuffer &CommandBuffer;
    UniformBuffer &UniformBuffer;
    DescriptorSet DescriptorSet;
    TimerPool &TimerPool;
};

struct UniformConstants {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

class ComputeSample
{
  public:
    ComputeSample();
    ~ComputeSample();

    void RunRenderLoop();
  private:
    DepthAttachment& CreateSwapchainDepthAttachment();
    UniformConstants GetUniforms();
    RasterPipeline LoadShaderPipeline(VulkanDevice &vulkanDevice, const RenderPass& renderPass) const;
    void RecordFrame(PerFrameState& state);
    std::vector<std::reference_wrapper<Semaphore>> CreateSemaphorePerInFlightFrame();
    std::vector<PerFrameState> CreatePerFrameState(VulkanDevice &vulkanDevice);
    std::vector<Vertex> GetVertices() const;
    std::vector<uint32_t> GetIndices() const;
    const DescriptorSetLayout& BuildDescriptorSetLayout(VulkanDevice &vulkanDevice) const;

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    DepthAttachment &m_DepthAttachment;
    RenderPass m_MainPass;
    const SwapchainFramebuffer& m_SwapchainFramebuffers;
    const DescriptorSetLayout &m_DescriptorSetLayout;
    std::vector<PerFrameState> m_PerFrameState;
    RasterPipeline m_RenderFullscreen;
    uint32_t m_CurrentFrameIndex = 0;
    Swapchain &m_Swapchain;
};
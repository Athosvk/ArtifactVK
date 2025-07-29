# ArtifactVK

ArtifactVK is meant to be a continuation of the Artifact-based engine and tools, providing a Vulkan backend to it.

Right now it's a simple app that's able to load models in a spec-compliant manner (no validation errors), with the use of dedicated transfer queues when available for uploading
of persistent buffers (index/vertex buffer). Window and swapchain management is handled for you.

## Usage
### Loading Raster Pipelines

```c++
auto builder = RasterPipelineBuilder("spirv/shaders/triangle.vert.spv", "spirv/shaders/triangle.frag.spv");
builder.SetVertexBindingDescription(Vertex::GetVertexBindingDescription());
builder.AddUniformBuffer(m_PerFrameState.front().UniformBuffer);
vulkanDevice.CreateRasterPipeline(std::move(builder), renderPass);
```

### Creating Uniform Buffers

```c++
auto& uniformBuffer = vulkanDevice.CreateUniformBuffer<UniformConstants>();
```

### Loading Images
```c++
Image image("textures/texture.jpg");
m_VulkanInstance.GetActiveDevice().CreateTexture(image.GetTextureCreateDesc());
```

### Binding Descriptors
First creating a layout that just tells your pipeline what you will be binding later on:

```c++
vulkanDevice.CreateDescriptorSetLayout(DescriptorSetBuilder().AddUniformBuffer().AddTexture());
```

To which you can then bind specific buffers:

```c++
state.DescriptorSet.BindUniformBuffer(state.UniformBuffer).BindTexture(m_Texture);
```

### Sample Frame Render
```c++
m_VulkanInstance.GetActiveDevice().AcquireNext(state.ImageAvailable);
state.CommandBuffer.WaitFence();
state.CommandBuffer.Begin();
auto uniforms = GetUniforms();
state.UniformBuffer.UploadData(GetUniforms());
auto bindSet = state.DescriptorSet.BindUniformBuffer(state.UniformBuffer).BindTexture(m_Texture);
state.CommandBuffer.DrawIndexed(m_SwapchainFramebuffers.GetCurrent(), m_MainPass, m_RenderFullscreen, m_VertexBuffer, m_IndexBuffer, 
    std::move(bindSet));

state.CommandBuffer.End(std::span{ &state.ImageAvailable, 1 }, std::span{ &state.RenderFinished, 1 });

m_VulkanInstance.GetActiveDevice().Present(std::span{&state.RenderFinished, 1});
```


## Samples
![image](content/textured_sample.gif)

![image](content/cube.png =450x450)
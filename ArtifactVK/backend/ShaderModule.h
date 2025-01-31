#pragma once
#include <filesystem>

#include <vulkan/vulkan.h>

class ShaderModule
{
  public:
    ShaderModule(const VkDevice& vkDevice, const std::vector<char>& bytes);
    ~ShaderModule();

    static ShaderModule LoadFromDisk(const VkDevice& vkDevice, const std::filesystem::path &filename);

  private:
    const VkDevice &m_Device;
    VkShaderModule m_ShaderModule;
};

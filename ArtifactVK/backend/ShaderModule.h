#pragma once
#include <filesystem>

#include <vulkan/vulkan.h>

class ShaderModule
{
  public:
    ShaderModule(const VkDevice& vkDevice, const std::vector<char>& bytes);
    ShaderModule(const ShaderModule &) = delete;
    ShaderModule(ShaderModule &&other);
    ~ShaderModule();

    ShaderModule &operator=(const ShaderModule &) = delete;
    ShaderModule &operator=(ShaderModule &&) = delete;

    static ShaderModule LoadFromDisk(const VkDevice& vkDevice, const std::filesystem::path &filename);

  private:
    const VkDevice &m_Device;
    VkShaderModule m_ShaderModule;
};

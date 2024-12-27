#pragma once
#include <span>
#include <string_view>
#include <unordered_map>

enum class EDeviceExtension
{
    VkSwapchain,
    Unknown
};

class DeviceExtensionMapping
{
  public:
    DeviceExtensionMapping();

    EDeviceExtension At(std::string_view extensionName) const;
    std::vector<const char *> ReverseMap(std::span<const EDeviceExtension> extensions) const;

  private:
    static std::unordered_map<std::string_view, EDeviceExtension> CreateNameMapping();

    std::unordered_map<std::string_view, EDeviceExtension> m_NameMapping;
    std::unordered_map<EDeviceExtension, std::string_view> m_ReverseMapping;
};

#pragma once
#include <unordered_map>
#include <string_view>

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
private:
	static std::unordered_map<std::string_view, EDeviceExtension> CreateNameMapping();

	std::unordered_map<std::string_view, EDeviceExtension> m_NameMapping;
};


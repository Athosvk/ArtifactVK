#pragma once
#include <unordered_map>
#include <string_view>

enum EDeviceExtension
{
	VkSwapchain
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


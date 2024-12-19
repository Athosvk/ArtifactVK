#pragma once
#include <unordered_map>
#include <string_view>
#include <span>

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
	const char* ReverseMap(std::span<const EDeviceExtension> extensions) const;
private:
	static std::unordered_map<std::string_view, EDeviceExtension> CreateNameMapping();

	std::unordered_map<std::string_view, EDeviceExtension> m_NameMapping;
	std::unordered_map<EDeviceExtension, std::string_view> m_ReverseMapping;
};


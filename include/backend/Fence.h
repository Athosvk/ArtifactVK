#pragma once
#include <vulkan/vulkan.h>

#include <string>

class ExtensionFunctionMapping;

enum class FenceStatus
{
    // Without knowing the function to pass it into, we don't know if the fence is ever entering an unsignaled state
	UnsignaledOrReset,
	Signaled,
	Reset,
};

class Fence
{
  public:
    explicit Fence(VkDevice device);
    Fence(VkDevice device, bool startSignaled);
    Fence(const Fence &) = delete;
    Fence(Fence&& other);
    
    ~Fence();
    /// <summary>
    /// Waits for the fence and immediately resets it for future usage
    /// </summary>
    void WaitAndReset();
    /// <summary>
    /// Gets the underlying fence for usage in calls to the Vulkan API
    /// </summary>
    /// <returns>The underlying fence handle</returns>
    /// <remarks>
    /// Never keep the fence longer than the function call to the API,
    /// as this can poison the fence status
    /// </remarks>
    VkFence Get() const;
    FenceStatus QueryStatus();
    bool WasReset() const;
    void SetName(const std::string& name, const ExtensionFunctionMapping& functionMapping) const;
  private:
    VkFence m_Fence;
    VkDevice m_Device;
    mutable FenceStatus m_Status;
};

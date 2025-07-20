#pragma once
#include <functional>
#include <optional>

#include "Queue.h"

class DeviceBuffer;
class Texture;

struct QueueSpecifier
{
    Queue SourceQueue;
    Queue DestionationQueue;
};

struct BufferMemoryBarrierElement
{
    std::reference_wrapper<DeviceBuffer> Buffer;
    std::optional<QueueSpecifier> Queues;
    VkAccessFlags SourceAccessMask;
    VkAccessFlags DestinationAccessMask;
};

struct BufferMemoryBarrier {
    BufferMemoryBarrierElement Barrier;
    VkPipelineStageFlags SourceStageMask;
    VkPipelineStageFlags DestinationStageMask;
};

struct ImageMemoryBarrierElement
{
    std::reference_wrapper<Texture> Image;
    std::optional<QueueSpecifier> Queues;
    VkAccessFlags SourceAccessMask;
    VkAccessFlags DestinationAccessMask;
    VkImageLayout SourceLayout;
    VkImageLayout DestinationLayout;
};

struct ImageMemoryBarrier
{
    ImageMemoryBarrierElement Barrier;
    VkPipelineStageFlags SourceStageMask;
    VkPipelineStageFlags DestinationStageMask;
};

struct MemoryBarrierArray {
    MemoryBarrierArray(BufferMemoryBarrier&& barrier);
    MemoryBarrierArray(ImageMemoryBarrier&& barrier);

    std::vector<BufferMemoryBarrierElement> BufferBarriers;
    std::vector<ImageMemoryBarrierElement> ImageBarriers;
    VkPipelineStageFlags SourceStageMask;
    VkPipelineStageFlags DestinationStageMask;
};

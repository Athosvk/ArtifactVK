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

struct BufferMemoryBarrierArray {
    BufferMemoryBarrierArray(BufferMemoryBarrier&& barrier);

    std::vector<BufferMemoryBarrierElement> Barriers;
    VkPipelineStageFlags SourceStageMask;
    VkPipelineStageFlags DestinationStageMask;
};

struct ImageMemoryBarrier
{
    std::reference_wrapper<Texture> Image;
    std::optional<QueueSpecifier> Queues;
    VkAccessFlags SourceAccessMask;
    VkAccessFlags DestinationAccessMask;
    VkImageLayout SourceLayout;
    VkImageLayout DestinationLayout;
    VkPipelineStageFlags SourceStageMask;
    VkPipelineStageFlags DestinationStageMask;
};

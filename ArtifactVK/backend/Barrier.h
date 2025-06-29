#pragma once
#include <functional>

#include "Queue.h"

class DeviceBuffer;

struct BufferMemoryBarrierElement
{
    std::reference_wrapper<DeviceBuffer> Buffer;
    Queue SourceQueue;
    Queue DestinationQueue;
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

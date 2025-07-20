#include "Barrier.h"

MemoryBarrierArray::MemoryBarrierArray(BufferMemoryBarrier &&barrier)
    : SourceStageMask(barrier.SourceStageMask), DestinationStageMask(barrier.DestinationStageMask),
      BufferBarriers({barrier.Barrier})
{
}

MemoryBarrierArray::MemoryBarrierArray(ImageMemoryBarrier &&barrier)
    : SourceStageMask(barrier.SourceStageMask), DestinationStageMask(barrier.DestinationStageMask),
      ImageBarriers({barrier.Barrier})
{
}

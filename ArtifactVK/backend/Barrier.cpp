#include "Barrier.h"

BarrierArray::BarrierArray(BufferMemoryBarrier &&barrier)
    : SourceStageMask(barrier.SourceStageMask), DestinationStageMask(barrier.DestinationStageMask),
      BufferBarriers({barrier.Barrier})
{
}

BarrierArray::BarrierArray(ImageMemoryBarrier &&barrier)
    : SourceStageMask(barrier.SourceStageMask), DestinationStageMask(barrier.DestinationStageMask),
      ImageBarriers({barrier.Barrier})
{
}

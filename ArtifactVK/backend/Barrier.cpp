#include "Barrier.h"

BufferMemoryBarrierArray::BufferMemoryBarrierArray(BufferMemoryBarrier &&barrier)
    : SourceStageMask(barrier.SourceStageMask), DestinationStageMask(barrier.DestinationStageMask), Barriers({barrier.Barrier})
{
}

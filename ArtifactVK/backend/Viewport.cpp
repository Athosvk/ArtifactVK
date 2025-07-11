#include "Viewport.h"

float Viewport::AspectRatio() const
{
    return Viewport.width / static_cast<float>(Viewport.height);
}

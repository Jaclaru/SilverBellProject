#include "BeforeRender.hh"

SilverBell::BeforeRenderEvent& SilverBell::BeforeRenderEvent::Instance()
{
    static BeforeRenderEvent Instance;
    return Instance;
}

#include "BeforeRender.hh"

using namespace SilverBell;

BeforeRenderEvent& BeforeRenderEvent::Instance()
{
    static BeforeRenderEvent sInstance;
    return sInstance;
}

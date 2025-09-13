#pragma once

#include "InternalLibMarco.hh"

#include "CallbackSystem.hh"
#include "Mixins.hh"

namespace SilverBell
{
    struct TimeInfo
    {
        double RenderStartTime = 0.0;
        double LastTickTime = 0.0;
        double CurrentTime = 0.0;
        double DeltaTime = 0.0;
        double WorldTime = 0.0;
    };

    class INTERNALLIB_API BeforeRenderEvent : public TMP::GenericCallbackSystem<std::variant<TimeInfo*>>,
                                              public NonCopyable, public NonMovable
    {
    public:
        static BeforeRenderEvent& Instance();

    private:
        BeforeRenderEvent() = default;
        ~BeforeRenderEvent() = default;
    };
}

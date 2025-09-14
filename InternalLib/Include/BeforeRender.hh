#pragma once

#include "CallbackSystem.hh"
#include "InternalLibMarco.hh"
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

    class INTERNALLIB_API BeforeRenderEvent : public TMP::GenericCallbackSystem<std::variant<TimeInfo*>>,  // NOLINT(cppcoreguidelines-special-member-functions)
                                              public NonCopyable, public NonMovable
    {
    public:
        static BeforeRenderEvent& Instance();

    private:
        BeforeRenderEvent() = default;
        ~BeforeRenderEvent() = default;
    };
}

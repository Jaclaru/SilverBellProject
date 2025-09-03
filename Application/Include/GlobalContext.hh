#pragma once

#include "Mixins.hh"

namespace SilverBell::Application
{
    class FApplication;

    // 全局上下文，单例模式
    class GlobalContext : public NonCopyable, public NonMovable
    {
    public:
        static GlobalContext& Instance();

        __FORCEINLINE void SetMainWindow(FApplication* InMainWindow)
        {
            MainWindow = InMainWindow;
        }

        __FORCEINLINE FApplication* GetMainWindow() const
        {
            return MainWindow;
        }

    private:
        GlobalContext() = default;
        ~GlobalContext() = default;

        FApplication* MainWindow;
    };

}

#pragma once

#include "Mixins.hh"

namespace SilverBell::Application
{
    class FApplication;

    class GlobalContext : public NonCopyable
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
        GlobalContext();
        ~GlobalContext();

        FApplication* MainWindow;

    };

}

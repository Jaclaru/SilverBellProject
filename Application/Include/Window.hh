#pragma once

#include <QWindow>

namespace SilverBell::Application
{
    class FWindow : public QWindow
    {
        Q_OBJECT

    public:

        FWindow();

        ~FWindow() override;

    private:
        void InitializeRenderer();
        //void createSurface();
        //void cleanup();
        // Add other Vulkan-related methods and members as needed

    };
}

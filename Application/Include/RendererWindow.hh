#pragma once

#include <QWindow>
#include <QWidget>

#include <memory>

namespace SilverBell::Renderer
{
    class FVulkanRenderer;
}

namespace SilverBell::Application
{
    class FRendererWindow : public QWindow
    {
        Q_OBJECT
    public:

        FRendererWindow();
        ~FRendererWindow();

        void exposeEvent(QExposeEvent*) override;

        void Render();

        bool event(QEvent* Event) override;

    private:

        void InitializeVulkanRenderer();

        std::unique_ptr<Renderer::FVulkanRenderer> Renderer;

        bool bInitialzed;
    };

    class FRenderWidget : public QWidget
    {
        Q_OBJECT

    public:
        FRenderWidget(QWidget* Parent);
        ~FRenderWidget() override;
    };

}

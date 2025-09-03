#pragma once

#include <QThread>
#include <QWindow>
#include <QWidget>

#include <memory>

namespace SilverBell::Renderer
{
    class FVulkanRenderer;
}

namespace SilverBell::Application
{

    class FRendererThread : public QThread
    {
        Q_OBJECT
    public:
        FRendererThread(SilverBell::Renderer::FVulkanRenderer* renderer, QObject* parent = nullptr)
            : QThread(parent), Renderer(renderer), bRunning(true)
        {
        }

        void run() override;

        void Stop() { bRunning = false; }

    signals:

        void UpdateFPS(double iFPS);

    private:
        Renderer::FVulkanRenderer* Renderer;
        std::atomic<bool> bRunning;
    };

    class FRendererWindow : public QWindow
    {
        Q_OBJECT
    public:

        FRendererWindow();
        ~FRendererWindow() override;

        void exposeEvent(QExposeEvent*) override;

        void Render();

        //bool event(QEvent* Event) override;

    private:

        void InitializeVulkanRenderer();

        std::unique_ptr<Renderer::FVulkanRenderer> Renderer;

        bool bInitialzed;

        std::unique_ptr<FRendererThread> RenderThread;
    };

    //class FRenderWidget : public QWidget
    //{
    //    Q_OBJECT

    //public:
    //    FRenderWidget(QWidget* Parent);
    //    ~FRenderWidget() override;
    //};

}

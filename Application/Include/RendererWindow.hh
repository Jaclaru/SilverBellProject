#pragma once

#include "BeforeRender.hh"

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
        FRendererThread(Renderer::FVulkanRenderer* renderer, QObject* parent = nullptr);

        void run() override;

        void Stop() { bRunning = false; }

        bool IsRunning() const { return bRunning; }

        void RequestResize(int w, int h) { bNeedResize = true; NewWidth = w; NewHeight = h; }

    signals:

        void UpdateFPS(double iFPS);

    private:
        Renderer::FVulkanRenderer* Renderer;
        std::atomic<bool> bRunning;
        std::atomic<bool> bNeedResize;
        int NewWidth, NewHeight;

        TimeInfo TimeInformation;
    };

    class FRendererWindow : public QWindow
    {
        Q_OBJECT
    public:

        FRendererWindow();
        ~FRendererWindow() override;

        void exposeEvent(QExposeEvent*) override;

        void resizeEvent(QResizeEvent*) override;

        void HandleResize();

    private:

        void InitializeVulkanRenderer();


        std::unique_ptr<Renderer::FVulkanRenderer> Renderer;

        bool bInitialzed;

        std::unique_ptr<FRendererThread> RenderThread;

        QSize lastSize;

        QTimer* ResizeTimer;

        bool bFirstResize;
    };

    //class FRenderWidget : public QWidget
    //{
    //    Q_OBJECT

    //public:
    //    FRenderWidget(QWidget* Parent);
    //    ~FRenderWidget() override;
    //};

}

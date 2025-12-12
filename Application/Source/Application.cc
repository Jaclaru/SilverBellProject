#include "Application.hh"

#include "GlobalContext.hh"
#include "RendererWindow.hh"

#include <QVulkanWindow>
#include <QTimer>
#include <QProcess>

using namespace SilverBell::Application;

FApplication::FApplication()
    : QMainWindow()
{
    setWindowTitle("Silver Bell Engine");
    resize(800, 600);

    FRendererWindow* RendererWindow = new FRendererWindow();

    QWidget* Container = QWidget::createWindowContainer(RendererWindow, this);
    setCentralWidget(Container);

    GlobalContext::Instance().SetMainWindow(this);

    // 定时器每0.2秒更新当前渲染帧率和内存占用
    /*QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, RendererWindow]() 
        {
        setWindowTitle(QString("Silver Bell Engine - FPS: %1").arg(RendererWindow->GetFPS(), 0, 'f', 2));
        });
    timer->start(200);*/
}

FApplication::~FApplication()
{

}

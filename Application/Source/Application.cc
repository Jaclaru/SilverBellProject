#include "Application.hh"

#include "RendererWindow.hh"

using namespace SilverBell::Application;
#include <QVulkanWindow>


FApplication::FApplication()
    : QMainWindow()
{
    setWindowTitle("Silver Bell Engine");
    resize(800, 600);

    FRendererWindow* RendererWindow = new FRendererWindow();

    QWidget* Container = QWidget::createWindowContainer(RendererWindow, this);
    setCentralWidget(Container);

    Container->show();
}

FApplication::~FApplication()
{

}



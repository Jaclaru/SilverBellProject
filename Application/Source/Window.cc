#include "Window.hh"

#include <QVulkanWindowRenderer>

using namespace SilverBell::Application;

FWindow::FWindow()
{
    setSurfaceType(VulkanSurface);
    setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    setGeometry(100, 100, 800, 600);
    setTitle("Vulkan Window");
}

FWindow::~FWindow()
{
    // Cleanup code if needed
}

void FWindow::InitializeRenderer()
{

}

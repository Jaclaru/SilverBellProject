#include "RendererWindow.hh"

#include "VulkanRenderer.hh"
using namespace SilverBell::Renderer;
using namespace SilverBell::Application;

#include <QVulkanInstance>
#include <QDebug>

#include <unordered_set>
namespace
{
    std::vector InstanceExtensions =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME, // Windows平台
#endif
    };
}

FRendererWindow::FRendererWindow()
    : Renderer(nullptr),
      bInitialzed(false)
{
    setWidth(800);
    setHeight(600);

    setSurfaceType(VulkanSurface);
}

FRendererWindow::~FRendererWindow()
{
    
}

void FRendererWindow::exposeEvent(QExposeEvent*)
{
    if (isExposed())
    {
        if (!bInitialzed)
        {
            // 要在expose之后初始化
            // 因为需要窗口句柄HWND
            InitializeVulkanRenderer();
            bInitialzed = true;
        }
        Render();
    }
}

void FRendererWindow::Render()
{
    if (Renderer == nullptr)return;

    Renderer->DrawFrame();
    requestUpdate();
}

bool FRendererWindow::event(QEvent* Event)
{
    switch (Event->type()) {
    case QEvent::UpdateRequest:
        Render();
        break;

        // The swapchain must be destroyed before the surface as per spec. This is
        // not ideal for us because the surface is managed by the QPlatformWindow
        // which may be gone already when the unexpose comes, making the validation
        // layer scream. The solution is to listen to the PlatformSurface events.
    //case QEvent::PlatformSurface:
    //    if (static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
    //        d->releaseSwapChain();
    //        d->reset();
    //    }
    //    break;

    default:
        break;
    }

    return QWindow::event(Event);
}

void FRendererWindow::InitializeVulkanRenderer()
{
    // 获取QT需要的vulkan 扩展
    QVulkanInstance tQVulkanInstance;
    tQVulkanInstance.create();

    // 获取 Qt 需要的 Vulkan 扩展
    auto extensions = tQVulkanInstance.supportedExtensions();

    std::unordered_set InstanceExtSet(InstanceExtensions.begin(), InstanceExtensions.end());
    // 打印extensions
    for (const auto& ext : extensions) {
        qDebug() << "Supported Vulkan Extension:" << ext.name;
        InstanceExtSet.insert(ext.name);
    }

    std::unique_ptr tRenderer = std::make_unique<FVulkanRenderer>();
    tRenderer->SetRequiredInstanceExtensions(std::vector(InstanceExtSet.begin(), InstanceExtSet.end()).data(), static_cast<int>(InstanceExtSet.size()));
    tRenderer->CreateInstance();

    tRenderer->CreateSurface((void*)this->winId());
    tRenderer->PickPhysicalDevice();
    tRenderer->CreateLogicalDevice();
    tRenderer->CreateSwapChain(this->width(), this->height());
    tRenderer->CreateImageViews();
    tRenderer->CreateRenderPass();
    tRenderer->CreateGraphicsPipeline();
    tRenderer->CreateFramebuffers();
    tRenderer->CreateCommandPool();
    tRenderer->CreateCommandBuffers();
    tRenderer->CreateSemaphores();

    Renderer = std::move(tRenderer);
}

//FRenderWidget::FRenderWidget(QWidget* Parent)
//{
//    QWindow* Window = new FRendererWindow();
//    QWidget* Container = QWidget::createWindowContainer(Window, this);
//
//    QVBoxLayout
//}

//FRenderWidget::~FRenderWidget()
//{
//}

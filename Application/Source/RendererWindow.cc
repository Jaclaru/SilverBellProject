#include "RendererWindow.hh"

#include "VulkanRenderer.hh"

#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QVulkanInstance>

#include <spdlog/spdlog.h>

#include <unordered_set>

#include "Application.hh"
#include "GlobalContext.hh"

using namespace SilverBell::Renderer;
using namespace SilverBell::Application;

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

void FRendererThread::run()
{
    while (bRunning)
    {
        if (Renderer->DrawFrame())
        {
            // 帧率统计
            static int FrameCount = 0;
            static double LastTime = 0.0;
            static double FPS = 0.0;
            ++FrameCount;
            double currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            if (LastTime == 0.0) LastTime = currentTime;

            if (currentTime - LastTime >= 1.0) // 每秒统计一次
            {

                FPS = FrameCount / (currentTime - LastTime);
                FrameCount = 0;
                LastTime = currentTime;
                emit UpdateFPS(FPS);
            }
        }
        // msleep(16); // 控制帧率，约60FPS
    }
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
    if (RenderThread)
    {
        RenderThread->Stop();
        RenderThread->wait();
    }
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
    }
}

void FRendererWindow::Render()
{
    if (Renderer == nullptr) return;

    // 这里存在一个bug.由于这里做的递归requestUpdate
    // 窗口大小变化时，Qt 会频繁发送重绘/刷新事件，如 QEvent::UpdateRequest，
    // 形成“自激”渲染循环，导致 Render() 被高频调用。
    // 这会引发性能问题，甚至导致应用程序无响应。
    // 已改完通过渲染线程持续渲染

    // 成功绘制一帧后继续请求下一帧
    if (Renderer->DrawFrame())
    {
        requestUpdate();
    }
}

void FRendererWindow::InitializeVulkanRenderer()
{
    // 获取QT需要的vulkan 扩展
    QVulkanInstance tQVulkanInstance;
    tQVulkanInstance.create();
    auto extensions = tQVulkanInstance.supportedExtensions();

    std::unordered_set InstanceExtSet(InstanceExtensions.begin(), InstanceExtensions.end());
    // 打印extensions
    for (const auto& ext : extensions) {
        qDebug() << "Supported Vulkan Extension:" << ext.name;
        // 这个扩展用于嵌入式系统、特定硬件调试或多驱动环境，​并非所有 Vulkan 实现都支持，暂时跳过。
        if (ext.name == "VK_LUNARG_direct_driver_loading")
        {
            continue;
        }
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
    tRenderer->CreateVertexBuffers();
    tRenderer->CreateCommandBuffers();
    tRenderer->CreateSemaphores();

    Renderer = std::move(tRenderer);

    // 启动渲染线程
    RenderThread = std::make_unique<FRendererThread>(Renderer.get());
    RenderThread->start();
    // 连接信号槽，更新FPS显示
    if (FApplication* MainWindow = GlobalContext::Instance().GetMainWindow())
    {
        connect(RenderThread.get(), &FRendererThread::UpdateFPS, MainWindow, &FApplication::OnUpdateFPS);
    }
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

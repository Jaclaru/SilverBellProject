
#include "Application.hh"

#include <QApplication>

#include "Logger.hh"

int main(int Argc, char** Argv)
{
    // 初始化日志
	SilverBell::Utility::Logger::InitializeAsyncLogging();

    QApplication QApp(Argc, Argv);

    SilverBell::Application::FApplication Application;
    Application.show();

    LOG_INFO("中文日志测试：{}", "你好，世界！");

    return QApplication::exec();
}

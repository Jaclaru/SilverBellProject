
#include "Application.hh"

#include <QApplication>

// 让spdlog使用动态链接，减小体积
#define SPDLOG_COMPILED_LIB
#include <spdlog/spdlog.h>

int main(int Argc, char** Argv)
{
    // 设置系统本地化信息，防止中文乱码
    setlocale(LC_ALL, "");

    QApplication QApp(Argc, Argv);

    SilverBell::Application::FApplication Application;
    Application.show();


    spdlog::info("中文日志测试：{}", "你好，世界！");

    return QApp.exec();
}

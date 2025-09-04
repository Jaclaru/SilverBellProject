
#include "Application.hh"

#include <QApplication>

#include <spdlog/spdlog.h>

int main(int Argc, char** Argv)
{
    QApplication QApp(Argc, Argv);

    SilverBell::Application::FApplication Application;
    Application.show();

    // 由于编译的spdlog动态库勾选了宽字符和utf-8支持，所以可以直接打印中文
    spdlog::info("中文日志测试：{}", "你好，世界！");

    return QApp.exec();
}

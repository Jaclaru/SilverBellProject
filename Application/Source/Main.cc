#include <QApplication>

#include <Application.hh>

int main(int Argc, char** Argv)
{
    QApplication QApp(Argc, Argv);

    SilverBell::Application::FApplication Application;
    Application.show();

    return QApp.exec();
}

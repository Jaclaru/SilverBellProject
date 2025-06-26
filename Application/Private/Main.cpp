#include <QApplication>

#include <QMainWindow>

int main(int Argc, char** Argv)
{

    QApplication App(Argc, Argv);

    QMainWindow MainWindow;
    MainWindow.setWindowTitle("My Application");

    QWidget* CentralWidget = new QWidget(&MainWindow);
    MainWindow.setCentralWidget(CentralWidget);

    MainWindow.resize(800, 600);
    MainWindow.show();
    

    return App.exec();
}
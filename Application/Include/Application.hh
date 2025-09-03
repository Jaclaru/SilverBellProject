#pragma once

#include <QMainWindow>

namespace SilverBell::Application
{
    // 程序主窗口
    class FApplication : public QMainWindow
    {
        Q_OBJECT

    public:

        FApplication();
        ~FApplication() override;

    public slots:
        void OnUpdateFPS(double iFPS)
        {
            setWindowTitle(QString("Silver Bell Engine - FPS: %1").arg(iFPS, 0, 'f', 2));
        }

    };

}

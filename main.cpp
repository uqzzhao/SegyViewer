
#include "mainwindow.h"
#include <QApplication>
#include <qpixmap.h>
#include <QSplashScreen>
#include <QElapsedTimer>
#include <QDesktopWidget>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::addLibraryPath("./plugins");

    //设置程序启动画面
    QPixmap pixmap(":/resources/start.png");
    QSplashScreen splash(pixmap);
    splash.show();

   // 设置画面停留时间为3秒
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed()<3000)
    {
        QCoreApplication::processEvents();
    }


    MainWindow w;
    w.setWindowTitle(QObject::tr("DeepListen SEGY Viewer"));
    w.show();
    w.move ((QApplication::desktop()->width() - w.width())/2,
            (QApplication::desktop()->height() - w.height())/2);

    splash.finish(&w);

    return a.exec();
}

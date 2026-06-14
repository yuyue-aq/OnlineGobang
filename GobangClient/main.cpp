#include "OnlineGobang.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    OnlineGobang window;
    window.show();
    return app.exec();
}

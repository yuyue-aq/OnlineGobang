#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    quint16 port = 9527;
    if (argc > 1) {
        bool ok = false;
        quint16 p = QString(argv[1]).toUShort(&ok);
        if (ok) port = p;
    }

    GobangServer server(port);
    if (!server.start()) {
        return 1;
    }

    return app.exec();
}

#include <QApplication>
#include <QCoreApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("StudentGame"));
    QCoreApplication::setApplicationName(QStringLiteral("game"));

    MainWindow window;
    window.show();
    return app.exec();
}

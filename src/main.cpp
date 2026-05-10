#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SC Editor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SCEditor");
    MainWindow w;
    w.showMaximized();
    return app.exec();
}

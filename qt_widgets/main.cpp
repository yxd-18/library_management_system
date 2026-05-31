#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Library Management System Qt");
    app.setOrganizationName("CourseDesign");

    MainWindow window;
    window.show();
    return app.exec();
}

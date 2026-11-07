#include <QApplication>
#include "gui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SCAPMS");
    QApplication::setStyle("Fusion");

    scapms::gui::MainWindow window;
    window.resize(1150, 700);
    window.show();
    return app.exec();
}
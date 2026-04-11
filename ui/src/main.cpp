#include <QApplication>
#include <QtPlugin>
#include "mainwindow.h"

// Plugin loadinfgg needed only on static build hence is
// wrong on non--windows builds
//
#ifdef Q_OS_WIN
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GraphAnalyzer");

    // Workaround: Qt 6.6.x bug on Windows — QWindowsCursor::createPixmapCursor
    // asserts "bm.format() == QImage::Format_Mono" when deriving cursor shape
    // from widget style on first mouse-enter event. Force-initializing the
    // arrow cursor here pre-warms the cursor cache and prevents the crash.
    QApplication::setOverrideCursor(Qt::ArrowCursor);
    QApplication::restoreOverrideCursor();

    int execResult = 0;
    {
        MainWindow window;
        window.show();
        execResult = app.exec();
    }
    return execResult;
}

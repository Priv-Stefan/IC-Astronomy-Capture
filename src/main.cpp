#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QDir>
#include <QLibraryInfo>
#include <QDebug>

#include "mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Application metadata used by QSettings
    QApplication::setOrganizationName("IC-Astronomy");
    QApplication::setApplicationName(APP_NAME);
    QApplication::setApplicationVersion(APP_VERSION);

    ic4::InitLibraryConfig conf = {};
    conf.apiLogLevel = ic4::LogLevel::Warning;
    conf.logTargets = ic4::LogTarget::WinDebug;
    conf.defaultErrorHandlerBehavior = ic4::ErrorHandlerBehavior::Throw;
    ic4::initLibrary(conf);


    // ----------------------------------------------------------------
    // Load language setting
    // ----------------------------------------------------------------
    QSettings settings;
    QString language = settings.value("options/language", QLocale::system().name().left(2)).toString();

    // Qt built-in translations (dialogs, buttons …)
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + language,
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
    {
        app.installTranslator(&qtTranslator);
    }

    // Application translations
    QTranslator appTranslator;
    // Search order: next to the executable, then :/translations resource
    QStringList searchPaths = {
        QApplication::applicationDirPath() + "/",
        ":/translations"
    };
    for (const QString &path : searchPaths) {
        if (appTranslator.load("ic_astronomy_" + language, path)) {
            app.installTranslator(&appTranslator);
            qDebug() << "Loaded translation:" << language << "from" << path;
            break;
        }
    }

    // ----------------------------------------------------------------
    // Show main window
    // ----------------------------------------------------------------
    MainWindow window;
    window.show();

    return app.exec();
}

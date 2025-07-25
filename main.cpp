#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "shortcut-emitter_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();

    // QHotkey test
    QHotkey hotkey(QKeySequence("Ctrl+Alt+1"), true, &a); //The hotkey will be automatically registered
    qDebug() << "Is registered:" << hotkey.isRegistered();

    QObject::connect(&hotkey, &QHotkey::activated, qApp, [&](){
        QMessageBox::information(&w, "Informasi", "Hotkey ditekan!\nCoba dengan kondisi lain");
        //qDebug() << "Hotkey Activated - try again in different case";
        //qApp->quit();
    });

    return a.exec();
}

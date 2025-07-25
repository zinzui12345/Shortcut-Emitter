#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QX11Info>

#include <X11/Xlib.h>               // Untuk mendapatkan akses ke X11 display
#include <X11/keysym.h>             // Untuk KeySym (kode tombol)
#include <X11/extensions/XTest.h>   // Mungkin tidak selalu dibutuhkan, tapi relevan untuk simulasi

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

        void setIP(QString value)
        {
            ip_address = value;
        }
        QString getIP()
        {
            return ip_address;
        }

    public slots:
        void handleConnectButton();

    private:
        Ui::MainWindow *ui;
        QString ip_address;
    };
#endif // MAINWINDOW_H

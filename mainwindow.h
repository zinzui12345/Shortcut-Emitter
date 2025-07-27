#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QX11Info>
#include <QHotkey>
#include <QWebSocket>
#include <QSslSocket>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QIcon>
#include <QStringListModel>

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

private slots:
    void handleConnectButton();

    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onSslErrors(const QList<QSslError> &errors);
    void onTextMessageReceived(const QString &message);

private:
    Ui::MainWindow *ui;

    QString ip_address;
    QWebSocket m_webSocket; // Objek QWebSocket
    QUrl m_url;             // URL koneksi WebSocket
    QStringListModel *hotkey_list;

    void setStatus(const QString &message, QString color="black");
};
#endif // MAINWINDOW_H

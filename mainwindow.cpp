#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->connectButton, &QPushButton::pressed, this, &MainWindow::handleConnectButton);
    QObject::connect(ui->ipAddress, &QLineEdit::textChanged, this, &MainWindow::setIP);

    ui->ipAddress->setText("127.0.0.1");
    ui->connectionStatus->setText("Terputus");
    ui->connectButton->setEnabled(true);

    connect(&m_webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);

    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        connect(&m_webSocket, &QWebSocket::errorChanged, this, [this](){
            onError(m_webSocket.error()); // Panggil slot onError dengan error saat ini
        });
    #else
        connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &MainWindow::onError);
    #endif

    connect(&m_webSocket, &QWebSocket::sslErrors, this, &MainWindow::onSslErrors);
}

void MainWindow::handleConnectButton()
{
    // Jika sedang terhubung, putuskan koneksi
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // Untuk Qt6, QWebSocket::Connected sudah benar dan langsung di namespace QWebSocket
        if (m_webSocket.isValid() && m_webSocket.state() == QWebSocket::Connected) {
            m_webSocket.close();
            return;
        }
    #else
        // Untuk Qt5, seringkali status enum berada di QAbstractSocket
        if (m_webSocket.isValid() && m_webSocket.state() == QAbstractSocket::ConnectedState) {
            m_webSocket.close();
            return;
        }
    #endif

    // Jika tidak terhubung, coba koneksikan
    QString ipAddress = MainWindow::getIP();
    QString port = ui->serverPort->text();
    if (ipAddress.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, "Kesalahan Input", "Alamat IP dan Port tidak boleh kosong.");
        return;
    }

    // Bentuk URL WebSocket
    // Gunakan "ws://" untuk koneksi non-SSL atau "wss://" untuk SSL
    m_url = QUrl(QString("%1%2:%3").arg(ui->protocolSelect->currentText()).arg(ipAddress).arg(port));
    //onTextMessageReceived("Menghubungkan ke: " + m_url.toString());
    m_webSocket.open(m_url);
}
void MainWindow::setStatus(const QString &message, QString color)
{
    ui->connectionStatus->setText(message);
}

void MainWindow::onConnected()
{
    setStatus("Terhubung");
    ui->connectButton->setText("Putuskan");
    ui->connectButton->setIcon(QIcon::fromTheme("network-offline"));
    ui->protocolSelect->setEnabled(false);
    ui->ipAddress->setEnabled(false);
    ui->serverPort->setEnabled(false);
}
void MainWindow::onDisconnected()
{
    setStatus("Terputus");
    ui->connectButton->setText("Sambungkan");
    ui->connectButton->setIcon(QIcon::fromTheme("network-wired"));
    ui->protocolSelect->setEnabled(true);
    ui->ipAddress->setEnabled(true);
    ui->serverPort->setEnabled(true);
}

void MainWindow::onError(QAbstractSocket::SocketError error)
{
    QMessageBox::critical(this, "Kesalahan", "Error: " + m_webSocket.errorString());
    qWarning() << "WebSocket Error:" << m_webSocket.errorString() << "(" << error << ")";
    setStatus("Error");
    ui->connectButton->setText("Sambungkan");
    ui->connectButton->setIcon(QIcon::fromTheme("network-wired"));
    ui->protocolSelect->setEnabled(true);
    ui->ipAddress->setEnabled(true);
    ui->serverPort->setEnabled(true);
}
void MainWindow::onSslErrors(const QList<QSslError> &errors)
{
    QString errorMessages;
    for (const QSslError &error : errors) {
        errorMessages += error.errorString() + "\n";
    }

    qWarning() << "SSL Errors encountered:" << errorMessages;
    QMessageBox::warning(this, "Kesalahan", "SSL Errors: " + errorMessages.trimmed());

    // --- Ini adalah baris KUNCI untuk mengabaikan semua error SSL ---
    m_webSocket.ignoreSslErrors();
    // -------------------------------------------------------------

    // Anda juga bisa secara selektif mengabaikan error tertentu jika perlu:
    // m_webSocket.ignoreSslErrors(QList<QSslError>() << QSslError(QSslError::HostNameMismatch));
}

void MainWindow::onTextMessageReceived(const QString &message)
{
    QMessageBox::information(this, "Menerima pesan", "Pesan: " + message);
}
// Contoh implementasi mengirim pesan
// void MainWindow::on_sendButton_clicked()
// {
//     QString message = ui->messageLineEdit->text();
//     if (!message.isEmpty()) {
//         m_webSocket.sendTextMessage(message);
//         logMessage("Sent: " + message);
//         ui->messageLineEdit->clear(); // Bersihkan input setelah kirim
//     }
// }

MainWindow::~MainWindow()
{
    if (m_webSocket.isValid()) {
        m_webSocket.close();
    }
    delete ui;
}

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

    hotkey_list = new QStringListModel(this);
    ui->listView->setModel(hotkey_list);
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
    // TODO : set label color
    ui->connectionStatus->setText(message);
}

void MainWindow::registerAllHotkeys()
{
    for (auto it = m_hotkeyConfigurations.constBegin(); it != m_hotkeyConfigurations.constEnd(); ++it) {
        QKeySequence sequence = it.key();
        QString hotkeyId = it.value();

        // Buat objek QHotkey baru
        QHotkey* hotkey = new QHotkey(sequence, true, getQApplicationInstance());

        // Tambahkan hotkey ke list kita
        m_hotkeys.append(hotkey);

        if (hotkey->isRegistered()) {
            qDebug() << "Hotkey '" << sequence.toString() << "' berhasil didaftarkan dengan ID:" << hotkeyId;
        } else {
            qWarning() << "Gagal mendaftarkan hotkey '" << sequence.toString() << "'. Mungkin sudah digunakan atau ada masalah izin.";
        }

        // --- Hubungkan sinyal activated dari QHotkey ke slot di MainWindow ---
        // Gunakan lambda untuk menangkap 'this' dan 'hotkeyId' (by value)
        // sehingga setiap hotkey memanggil slot dengan ID spesifiknya.
        QObject::connect(hotkey, &QHotkey::activated, this, [this, hotkeyId](){
            this->onHotkeyActivated(hotkeyId); // Panggil slot dengan ID hotkey
        });
        // --------------------------------------------------------------------
    }
}
void MainWindow::removeAllHotkeys()
{
    for (QHotkey* hotkey : m_hotkeys) {
        if (hotkey)
        {
            delete hotkey;
        }
    }
    m_hotkeys.clear();
    m_hotkeyConfigurations.clear();
}
void MainWindow::onHotkeyActivated(QString method)
{
    // QMessageBox::information(this, "Informasi dari Hotkey", "Hotkey " + method + " ditekan!");
    m_webSocket.sendTextMessage("{ \"message\": \"hotkey_request\", \"data\": \"" + method + "\", \"client_type\": \"shortcut_emitter\" }");
}

void MainWindow::onConnected()
{
    setStatus("Terhubung");
    ui->connectButton->setText("Putuskan");
    ui->connectButton->setIcon(QIcon::fromTheme("network-offline"));
    ui->protocolSelect->setEnabled(false);
    ui->ipAddress->setEnabled(false);
    ui->serverPort->setEnabled(false);
    m_webSocket.sendTextMessage("{ \"message\": \"connected\", \"client_type\": \"shortcut_emitter\" }");
}
void MainWindow::onDisconnected()
{
    setStatus("Terputus");
    hotkey_list->setStringList(QStringList());
    ui->connectButton->setText("Sambungkan");
    ui->connectButton->setIcon(QIcon::fromTheme("network-wired"));
    ui->protocolSelect->setEnabled(true);
    ui->ipAddress->setEnabled(true);
    ui->serverPort->setEnabled(true);
    MainWindow::removeAllHotkeys();
}

void MainWindow::onError(QAbstractSocket::SocketError error)
{
    QMessageBox::critical(this, "Kesalahan", "Error: " + m_webSocket.errorString());
    qWarning() << "WebSocket Error:" << m_webSocket.errorString() << "(" << error << ")";
    setStatus("Error");
    hotkey_list->setStringList(QStringList());
    ui->connectButton->setText("Sambungkan");
    ui->connectButton->setIcon(QIcon::fromTheme("network-wired"));
    ui->protocolSelect->setEnabled(true);
    ui->ipAddress->setEnabled(true);
    ui->serverPort->setEnabled(true);
    MainWindow::removeAllHotkeys();
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

    // bisa juga secara selektif mengabaikan error tertentu kalau memang perlu:
    // m_webSocket.ignoreSslErrors(QList<QSslError>() << QSslError(QSslError::HostNameMismatch));
}

void MainWindow::onTextMessageReceived(const QString &message)
{
    QByteArray jsonData = message.toUtf8();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << "Failed to parse JSON:" << parseError.errorString();
        QMessageBox::information(this, "Menerima pesan", "Pesan: " + message);
        return;
    }

    if (doc.isObject())
    {
        QString debugData;
        QJsonObject obj = doc.object();

        if (obj.contains("message") && obj.contains("data"))
        {
            if (obj.value("message").toString() == "set_shortcut" && obj.value("data").isArray())
            {
                QJsonArray dataArray = obj.value("data").toArray();

                hotkey_list->setStringList(QStringList());

                for (int i = 0; i < dataArray.size(); ++i)
                {
                    QJsonValue arrayElement = dataArray.at(i);

                    if (arrayElement.isObject())
                    {
                        QJsonObject hotkeyObject = arrayElement.toObject();
                        QString key;
                        QString method;

                        if (hotkeyObject.contains("key") && hotkeyObject.value("key").isString())
                        {
                            key = hotkeyObject.value("key").toString();
                        }
                        if (hotkeyObject.contains("method") && hotkeyObject.value("method").isString())
                        {
                            method = hotkeyObject.value("method").toString();
                        }

                        if (!key.isEmpty() && !method.isEmpty())
                        {
                            QStringList currentList = hotkey_list->stringList();
                            currentList.append(key + "\t -> " + method);
                            hotkey_list->setStringList(currentList);
                            m_hotkeyConfigurations[QKeySequence(key)] = method;
                        }
                    }
                }

                MainWindow::registerAllHotkeys();
                return;
            }
        }

        for (const QString& key : obj.keys()) {
            QJsonValue value = obj.value(key);
            qDebug() << "  Key:" << key << ", Value Type:" << value.type();
            debugData += "\n\n  Key: " + key + ", Value Type: " + value.type();

            // Akses nilai berdasarkan tipenya
            if (value.isString()) {
                qDebug() << "    (String):" << value.toString();
                debugData += "\n    (String): " + value.toString();
            } else if (value.isDouble()) { // Untuk angka (int, float, double)
                qDebug() << "    (Number):" << value.toDouble();
                debugData += "\n    (Number) :" + value.toString();
            } else if (value.isBool()) {
                qDebug() << "    (Boolean):" << value.toBool();
                debugData += &"\n    (Boolean) :" [ value.toBool() ];
            } else if (value.isObject()) {
                qDebug() << "    (Object):" << value.toObject();
                // Jika perlu, panggil fungsi rekursif atau proses lebih lanjut
                // QJsonObject nestedObj = value.toObject();
            } else if (value.isArray()) {
                qDebug() << "    (Array):" << value.toArray();
                debugData += "\n    (Array) :" + value.toString();
                // Jika perlu, panggil fungsi rekursif atau proses lebih lanjut
                // QJsonArray nestedArray = value.toArray();
            } else if (value.isNull()) {
                qDebug() << "    (Null)";
                debugData += "\n    (Null)";
            }
        }

        QMessageBox::information(this, "Menerima request", debugData);
    }
    else
    {
        QMessageBox::critical(this, "Kesalahan Input", "Properti `message` harus berupa Object!");
    }
}

QApplication* MainWindow::getQApplicationInstance()
{
    return qApp; // qApp adalah macro global untuk QApplication::instance()
    // Ini aman digunakan karena QApplication pasti sudah ada saat MainWindow dibuat.
}

MainWindow::~MainWindow()
{
    if (m_webSocket.isValid()) {
        m_webSocket.close();
    }
    removeAllHotkeys();
    delete ui;
}

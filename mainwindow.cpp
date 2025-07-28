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

void MainWindow::registerHotkey(const QString& keyString, const QString& methodId)
{
    QKeySequence sequence(keyString);

    if (sequence.isEmpty())
    {
        qWarning() << "Gagal mendaftarkan hotkey: Key string kosong atau tidak valid:" << keyString;
        return;
    }

    // Cek apakah hotkey sudah terdaftar atau ada di konfigurasi
    if (m_hotkeysMap.contains(sequence)) { // Cek di m_hotkeysMap sekarang
        qWarning() << "Hotkey '" << keyString << "' dengan ID '" << methodId
                   << "' sudah terdaftar. Lewati pendaftaran ulang.";
        return;
    }

    QHotkey* hotkey = new QHotkey(sequence, true, getQApplicationInstance());

    if (hotkey->isRegistered())
    {
        m_hotkeysMap[sequence] = hotkey; // Simpan di map dengan QKeySequence sebagai key
        m_hotkeyConfigurations[sequence] = methodId; // Tetap simpan methodId di sini
        qDebug() << "Hotkey '" << keyString << "' berhasil didaftarkan dengan ID:" << methodId;

        QObject::connect(hotkey, &QHotkey::activated, this, [this, methodId](){
            this->onHotkeyActivated(methodId);
        });
    }
    else
    {
        qWarning() << "Gagal mendaftarkan hotkey '" << keyString << "'. Mungkin sudah digunakan atau ada masalah izin.";
        delete hotkey;
    }
}
void MainWindow::removeHotkey(const QString& keyString)
{
    QKeySequence sequenceToRemove(keyString);

    if (sequenceToRemove.isEmpty())
    {
        qWarning() << "Tidak dapat menghapus hotkey: Key string kosong atau tidak valid:" << keyString;
        return;
    }

    // Langsung cari di m_hotkeysMap menggunakan QKeySequence sebagai key
    if (m_hotkeysMap.contains(sequenceToRemove))
    {
        QHotkey* hotkeyToDelete = m_hotkeysMap.take(sequenceToRemove); // take() akan menghapus entry dan mengembalikan value
        if (hotkeyToDelete) {
            delete hotkeyToDelete; // Hapus objek QHotkey dari memori
            m_hotkeyConfigurations.remove(sequenceToRemove); // Hapus dari konfigurasi

            qDebug() << "Hotkey '" << keyString << "' berhasil dihapus.";
            // QMessageBox::information(this, "Hotkey Dihapus", "Hotkey '" + keyString + "' berhasil dihapus.");
        }
    }
    else {
        qWarning() << "Hotkey '" << keyString << "' tidak ditemukan dalam daftar terdaftar.";
        // QMessageBox::warning(this, "Hotkey Tidak Ditemukan", "Hotkey '" + keyString + "' tidak ditemukan.");
    }
}
void MainWindow::removeAllHotkeys()
{
    qDebug() << "Menghapus semua hotkey yang terdaftar...";
    // Iterasi melalui QMap, bukan QList
    for (QHotkey* hotkey : m_hotkeysMap.values())
    { // Gunakan .values() untuk mendapatkan list QHotkey*
        if (hotkey) {
            delete hotkey;
        }
    }
    m_hotkeysMap.clear(); // Kosongkan map hotkey
    m_hotkeyConfigurations.clear(); // Kosongkan konfigurasi juga
    qDebug() << "Semua hotkey berhasil dihapus dan konfigurasi dikosongkan.";
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

                MainWindow::removeAllHotkeys();
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
                            MainWindow::registerHotkey(key, method);
                        }
                    }
                }

               return;
            }
            else if (obj.value("message").toString() == "add_shortcut" && obj.value("data").isArray())
            {
                QJsonArray dataArray = obj.value("data").toArray();

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
                            MainWindow::registerHotkey(key, method);
                        }
                    }
                }

                return;
            }
            else if (obj.value("message").toString() == "remove_shortcut" && obj.value("data").isArray())
            {
                QJsonArray dataArray = obj.value("data").toArray();

                for (int i = 0; i < dataArray.size(); ++i)
                {
                    QJsonValue arrayElement = dataArray.at(i);

                    if (arrayElement.isObject())
                    {
                        QJsonObject hotkeyObject = arrayElement.toObject();
                        QString key;

                        if (hotkeyObject.contains("key") && hotkeyObject.value("key").isString())
                        {
                            key = hotkeyObject.value("key").toString();
                        }

                        if (!key.isEmpty())
                        {
                            // FIXME : remove instead
                            //QStringList currentList = hotkey_list->stringList();
                            //currentList.append(key + "\t -> " + method);
                            //hotkey_list->setStringList(currentList);

                            QStringList currentList = hotkey_list->stringList();
                            QKeySequence sequenceToRemove(key);
                            QString itemToRemove = key + "\t -> " + m_hotkeyConfigurations.value(sequenceToRemove, ""); // Dapatkan string yang lengkap
                            // Perhatikan: m_hotkeyConfigurations.value(sequenceToRemove, "")
                            // Mungkin ini sudah dihapus di baris atas, jadi Anda perlu menyimpan methodId
                            // sebelum m_hotkeyConfigurations.remove(sequenceToRemove);
                            // Atau, lebih baik, ambil methodId dari m_hotkeyConfigurations sebelum menghapusnya.

                            // Ambil methodId sebelum menghapus dari m_hotkeyConfigurations
                            QString methodIdAssociated = m_hotkeyConfigurations.value(sequenceToRemove);
                            m_hotkeyConfigurations.remove(sequenceToRemove); // Pindahkan ke sini setelah mengambil methodId

                            QString fullItemToRemove = key + "\t -> " + methodIdAssociated;

                            // Cari dan hapus item dari QStringList
                            if (currentList.removeAll(fullItemToRemove) > 0) { // removeAll mengembalikan jumlah item yang dihapus
                                hotkey_list->setStringList(currentList); // Set kembali list yang sudah diupdate
                                qDebug() << "Hotkey '" << key << "' juga dihapus dari UI.";
                            } else {
                                qWarning() << "Hotkey '" << key << "' tidak ditemukan di UI meskipun terdaftar di backend.";
                            }
                            MainWindow::removeHotkey(key);
                        }
                    }
                }

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

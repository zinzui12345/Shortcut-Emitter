#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->connectButton, &QPushButton::pressed, this, &MainWindow::handleConnectButton);
    QObject::connect(ui->ipAddress, &QLineEdit::textChanged, this, &MainWindow::setIP);
}

void MainWindow::handleConnectButton()
{
    QString ip_addr = MainWindow::getIP();
    QMessageBox::information(this, "Informasi", "Tombol ditekan!\n" + ip_addr);
}

MainWindow::~MainWindow()
{
    delete ui;
}

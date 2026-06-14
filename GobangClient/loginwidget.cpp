#include "loginwidget.h"
#include "ui_loginwidget.h"

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    connect(ui->connectButton, &QPushButton::clicked,
            this, &LoginWidget::onConnectClicked);

    // 回车键触发连接
    connect(ui->nicknameEdit, &QLineEdit::returnPressed,
            this, &LoginWidget::onConnectClicked);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::setStatus(const QString &text, bool isError)
{
    ui->statusLabel->setText(text);
    ui->statusLabel->setStyleSheet(isError
        ? "color: red; font-size: 13px;"
        : "color: green; font-size: 13px;");
}

void LoginWidget::setConnecting(bool connecting)
{
    ui->connectButton->setEnabled(!connecting);
    ui->nicknameEdit->setEnabled(!connecting);
    ui->hostEdit->setEnabled(!connecting);
    ui->portEdit->setEnabled(!connecting);
    if (connecting) {
        ui->connectButton->setText("连接中...");
    } else {
        ui->connectButton->setText("连接");
    }
}

void LoginWidget::onConnectClicked()
{
    QString nickname = ui->nicknameEdit->text().trimmed();
    if (nickname.isEmpty()) {
        setStatus("请输入昵称", true);
        return;
    }

    QString host = ui->hostEdit->text().trimmed();
    if (host.isEmpty()) host = "127.0.0.1";

    bool ok = false;
    quint16 port = ui->portEdit->text().toUShort(&ok);
    if (!ok || port == 0) port = 9527;

    setConnecting(true);
    setStatus("正在连接服务器...", false);

    emit connectRequested(host, port, nickname);
}

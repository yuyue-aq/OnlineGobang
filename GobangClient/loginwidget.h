#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>

namespace Ui {
class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

    void setStatus(const QString &text, bool isError = false);
    void setConnecting(bool connecting);

signals:
    void connectRequested(const QString &host, quint16 port, const QString &nickname);

private slots:
    void onConnectClicked();

private:
    Ui::LoginWidget *ui;
};

#endif // LOGINWIDGET_H

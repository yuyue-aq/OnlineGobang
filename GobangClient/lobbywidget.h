#ifndef LOBBYWIDGET_H
#define LOBBYWIDGET_H

#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>

namespace Ui {
class LobbyWidget;
}

class LobbyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LobbyWidget(QWidget *parent = nullptr);
    ~LobbyWidget();

    void updateRoomList(const QJsonArray &rooms);
    void updatePlayerList(const QJsonArray &players);

signals:
    void createRoomRequested();
    void joinRoomRequested(const QString &roomId);
    void refreshRequested();

private slots:
    void onCreateRoomClicked();
    void onJoinRoomClicked();
    void onRoomDoubleClicked(int row, int column);

private:
    Ui::LobbyWidget *ui;
};

#endif // LOBBYWIDGET_H

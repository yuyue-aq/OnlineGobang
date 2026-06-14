#include "lobbywidget.h"
#include "ui_lobbywidget.h"

LobbyWidget::LobbyWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LobbyWidget)
{
    ui->setupUi(this);

    // 房间列表表头
    ui->roomTable->setColumnCount(4);
    ui->roomTable->setHorizontalHeaderLabels({"房间ID", "房间名", "人数", "状态"});
    ui->roomTable->horizontalHeader()->setStretchLastSection(true);
    ui->roomTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->roomTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->roomTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->createRoomButton, &QPushButton::clicked,
            this, &LobbyWidget::onCreateRoomClicked);
    connect(ui->joinRoomButton, &QPushButton::clicked,
            this, &LobbyWidget::onJoinRoomClicked);
    connect(ui->roomTable, &QTableWidget::cellDoubleClicked,
            this, &LobbyWidget::onRoomDoubleClicked);
}

LobbyWidget::~LobbyWidget()
{
    delete ui;
}

void LobbyWidget::updateRoomList(const QJsonArray &rooms)
{
    ui->roomTable->setRowCount(0);
    for (int i = 0; i < rooms.size(); i++) {
        QJsonObject room = rooms[i].toObject();
        int row = ui->roomTable->rowCount();
        ui->roomTable->insertRow(row);
        ui->roomTable->setItem(row, 0, new QTableWidgetItem(room["roomId"].toString()));
        ui->roomTable->setItem(row, 1, new QTableWidgetItem(room["roomName"].toString()));
        ui->roomTable->setItem(row, 2, new QTableWidgetItem(
            QString("%1/2").arg(room["playerCount"].toInt())));
        ui->roomTable->setItem(row, 3, new QTableWidgetItem(
            room["gameStarted"].toBool() ? "游戏中" : "等待中"));
    }
    ui->roomTable->resizeColumnsToContents();
}

void LobbyWidget::updatePlayerList(const QJsonArray &players)
{
    ui->playerList->clear();
    for (int i = 0; i < players.size(); i++) {
        QJsonObject player = players[i].toObject();
        ui->playerList->addItem(player["nickname"].toString());
    }
}

void LobbyWidget::onCreateRoomClicked()
{
    emit createRoomRequested();
}

void LobbyWidget::onJoinRoomClicked()
{
    int row = ui->roomTable->currentRow();
    if (row < 0) return;

    QString roomId = ui->roomTable->item(row, 0)->text();
    emit joinRoomRequested(roomId);
}

void LobbyWidget::onRoomDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row < 0) return;

    QString roomId = ui->roomTable->item(row, 0)->text();
    emit joinRoomRequested(roomId);
}

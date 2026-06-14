# TODO.md

# 基于 Socket 的联机五子棋系统

> 开发周期：7 天（课程设计版）

---

# 第一阶段：项目初始化 ✅

* [x] 创建 Server 工程
* [x] 创建 Client 工程
* [x] 配置 Qt Network 模块
* [x] 配置 Qt Widgets 模块
* [x] 建立 Git 仓库
* [x] 创建基础目录结构

---

# 第二阶段：服务器 ✅

## TCP服务器

* [x] 创建 QTcpServer
* [x] 监听端口
* [x] 接收客户端连接
* [x] 客户端断开处理

## 会话管理

* [x] ClientSession 类
* [x] 在线用户列表
* [x] 用户昵称管理

## 房间管理

* [x] 创建房间
* [x] 加入房间
* [x] 离开房间
* [x] 删除空房间

---

# 第三阶段：客户端 ✅

## 登录界面

* [x] 输入昵称
* [x] 连接服务器
* [x] 登录成功提示

## 大厅界面

* [x] 在线玩家列表
* [x] 房间列表
* [x] 创建房间
* [x] 加入房间

## 游戏界面

* [x] 棋盘绘制
* [x] 黑白棋绘制
* [x] 鼠标点击落子
* [x] 当前回合提示

---

# 第四阶段：通信协议 ✅

* [x] Login
* [x] CreateRoom
* [x] JoinRoom
* [x] LeaveRoom
* [x] Move
* [x] Chat
* [x] Ready
* [x] Restart
* [x] Surrender
* [x] UndoRequest
* [x] UndoReply
* [x] GameOver

统一采用 JSON 协议。

---

# 第五阶段：游戏逻辑 ✅

* [x] 棋盘数据结构
* [x] 落子合法性判断
* [x] 五连判断
* [x] 平局判断
* [x] 回合切换
* [x] 游戏结束处理

---

# 第六阶段：网络同步 ✅

* [x] 登录同步
* [x] 玩家匹配
* [x] 落子同步
* [x] 聊天同步
* [x] 游戏结束同步

---

# 第七阶段：功能完善 ✅

* [x] 悔棋
* [x] 认输
* [x] 再来一局
* [x] 房间退出
* [x] 玩家掉线处理
* [x] 自动返回大厅

---

# 第八阶段：UI优化

* [x] 棋盘美化
* [x] 棋子阴影
* [x] 聊天窗口
* [ ] 玩家头像（默认）
* [x] 状态栏
* [x] 胜利提示
* [x] 胜利连线显示

---

# 第九阶段：测试

* [ ] 单机双客户端测试
* [ ] 多客户端测试
* [ ] 掉线测试
* [ ] 非法落子测试
* [ ] 重连测试

---

# 第十阶段：课程设计材料

* [ ] 软件系统结构图
* [ ] 类图
* [ ] 时序图
* [ ] 网络协议说明
* [ ] 功能截图
* [ ] 关键代码整理
* [ ] 实验总结

---

# BUG修复记录 (2026-06-14)

## 编译错误修复
1. **`gobangboard.cpp`**: 移除不存在的 `#include <QGradientStops>` 头文件
2. **`OnlineGobang.h`**: 添加缺失的 `#include <QStackedWidget>`、`<QJsonArray>`、`<QJsonObject>`
3. **`networkmanager.h`**: 添加缺失的 `#include <QJsonArray>`（信号参数使用了 QJsonArray）
4. **`protocol.h`** (客户端+服务端): 添加 `#include <QJsonArray>`，修复默认参数 `= {}` → `= QJsonObject()` / `= QJsonArray()` 避免 initializer_list 歧义
5. **`gamewidget.cpp`**: 修复 `QString::arg(a, b)` 为链式调用 `.arg(a).arg(b)`

## 逻辑/功能修复
6. **RestartReply 未处理**: 服务端拒绝再来一局时发送 `RestartReply` 消息，客户端原先无法识别
   - 新增 `NetworkManager::restartReplyReceived` 信号
   - 新增 `OnlineGobang::onRestartReplyReceived` 槽
   - 新增 `GameWidget::onRestartReply` 方法（恢复准备按钮状态）
   - 新增 `Protocol::restartReplyResp()` 工厂方法
7. **胜利连线显示**: `GameOver` 消息包含 `winLine` 数据，客户端原先忽略
   - `gameOver` 信号增加 `QJsonArray` 参数贯穿 NetworkManager → OnlineGobang → GameWidget
   - `GameWidget::onGameOver` 解析 winLine 并调用 `GobangBoard::setWinLine()` 绘制
8. **服务端 `handleRestartReply`**: 使用新的 `Protocol::restartReplyResp()` 替代原始 makeMessage

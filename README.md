# C++ 多线程售票系统

大学生个人学习项目，使用 C++ 实现售票功能，逐步学习类封装、多线程、线程池、HTTP 接口和 SQLite 数据存储。

## 当前进度：V6 — SQLite 数据存储（进行中）

文档：[HTTP 接口说明](docs/API.md) · [V5 架构与流程（历史版本）](docs/V5-ARCHITECTURE.md)

已完成：

- 使用 Postman 发送 HTTP 请求，支持查询库存和购票。
- 使用线程池处理购票任务，通过互斥锁保护库存和票号。
- 使用 SQLite 保存库存、下一张票号和售票记录。
- 重启后从数据库恢复库存和下一张票号。
- 一次购票中的库存更新和记录插入放在同一事务中，失败时一起撤销。
- 将购票任务中的异常传回 HTTP 层，返回错误响应。

待完成：查询历史售票记录的数据库方法及 HTTP 接口。

## 当前接口

本机地址：`http://localhost:8080`。

| 方法 | 路径 | 功能 |
| --- | --- | --- |
| GET | `/health` | 检查服务是否运行 |
| GET | `/tickets` | 查询剩余票数 |
| POST | `/tickets/purchase` | 购买指定数量的票 |

Postman 购票时选择 **Body → raw → JSON**，输入：

```json
{
  "user_id": 1001,
  "ticket_count": 2
}
```

假设库存为 100、下一张票号为 1，成功时返回 HTTP 201：

```json
{
  "success": true,
  "message": "Purchase successful",
  "ticket_ids": [1, 2],
  "remaining_tickets": 98
}
```

参数错误返回 `400`，库存不足返回 `409`，购票中的数据库异常返回 `500`。

## 数据库

使用 SQLite，数据保存在 `ticket.db`，不需要单独启动数据库服务。

| 表名 | 字段 | 作用 |
| --- | --- | --- |
| `inventory` | `id`、`remaining_tickets`、`next_ticket_id` | 保存当前库存和下一张票号，使用 `id = 1` 的一条记录 |
| `sales` | `ticket_id`、`user_id`、`request_id`、`sale_time_ms` | 每张售出的票对应一条记录，时间保存为 Unix 时间戳（毫秒） |

首次运行时创建表，并在初始记录不存在时写入 100 张票、起始票号 1；已有库存不会被覆盖。`request_id` 是本次程序运行中生成的请求编号，重启后重新计数。

数据库文件使用相对路径，位于程序的工作目录。重启时应使用相同的工作目录，才能读取同一份数据。

## 购票流程

```text
Postman 发送请求
    → ApiServer 解析 JSON
    → TicketSystem 将购票任务交给 ThreadPool
    → 加锁，检查数量和库存，准备本次购票数据
    → TicketRepository.savePurchase() 保存到数据库
    → 保存成功后更新 C++ 内存中的库存和记录
    → 通过 promise / future 返回结果
    → ApiServer 返回 HTTP 响应
```

`savePurchase()` 内部使用同一个数据库连接完成事务：

```text
BEGIN IMMEDIATE
    → updateInventoryOnConnection() 更新库存和下一张票号
    → insertSaleOnConnection() 逐条插入本次售票记录
    → 全部成功：COMMIT
    → 执行失败：ROLLBACK，再向上抛出异常
```

例如购买两张票时，第二条记录写入失败，库存更新和第一条记录都会被撤销。数据库保存成功后才更新内存，避免购票失败却扣减内存库存。

## 项目结构

```text
CppTicketSystem.sln
README.md
docs/                             # 接口说明和历史架构图
TicketSystemCore/
├── main.cpp                      # 初始化数据库并启动服务
├── ApiServer.h / .cpp             # HTTP 请求和 JSON 响应
├── TicketSystem.h / .cpp          # 购票业务
├── ThreadPool.h / .cpp            # 任务队列和工作线程
├── TicketRepository.h / .cpp      # SQLite 数据库操作
├── SaleRecord.h                  # 售票记录结构
├── external/httplib.h             # HTTP 库
├── external/json.hpp              # JSON 库
├── external/sqlite3.c / .h        # SQLite 库
└── TicketSystemCore.vcxproj       # Visual Studio 项目配置
```

第三方库已随项目提供，保留其原有版权和许可说明。

## 构建与运行

使用 Visual Studio 2022，安装“使用 C++ 的桌面开发”，使用 MSVC v143 工具集和 Windows SDK。

1. 打开 `CppTicketSystem.sln`。
2. 选择 `Debug | x64` 或 `Release | x64`，生成解决方案。
3. 使用 Ctrl+F5 运行 `TicketSystemCore`，保持服务窗口开启。
4. 在 Postman 中访问上述接口。

服务使用 8080 端口，启动前确保端口未被占用。数据库初始化失败时会输出错误并停止启动。

## 已完成的测试

本阶段已进行手动接口测试和临时测试程序验证：

| 测试内容 | 结果 |
| --- | --- |
| 正常购票 | 返回 201，正确扣减库存并保存每张票的记录 |
| 数量无效、缺少字段、字段类型或 JSON 格式错误 | 返回 400，不扣减库存 |
| 库存不足 | 返回 409，不扣减库存 |
| 重启后查询并继续购票 | 库存保留，票号继续递增 |
| 数据库初始化失败 | 输出错误，停止启动 |
| 第二条售票记录故意写入失败 | 整笔事务回滚，数据库及内存状态保持原值 |
| 数据库故障经过 HTTP 接口返回 | 返回 500；移除故障后，同一服务可继续正常购票 |

故障测试使用独立临时数据库，未修改原有购票数据。临时测试程序未作为固定测试文件保存到仓库。

## 当前范围与下一步

当前用于本机、单个服务实例的学习和功能验证。尚未实现登录、退款、重复请求去重及售票记录查询接口；重复发送购票请求会再次购买。

下一步补充从数据库查询历史售票记录的功能，再通过 HTTP 接口返回。启动时目前只恢复库存和下一张票号，没有把历史售票记录加载到内存。

## 版本记录

V1–V6 是当前学习路线的阶段编号，早期 Git 编号与路线不同，保留已有历史。

| 阶段 | 主要内容 | 已有 Git 记录 |
| --- | --- | --- |
| V1 | 基础线程与共享票数 | 未单独提交 |
| V2 | 类封装、互斥锁、售票记录 | `6c52e77`，早期标签 `v1.0` |
| V3 | 请求队列、条件变量、生产者与消费者 | `3145b96`，早期标签 `v2.0` |
| V4 | 通用线程池 | `f774eef`，标签 `v4.0` |
| V5 | HTTP 接口、JSON、promise / future | `31ad72b`，标签 `v5.0` |
| V6 | SQLite 持久化、购票事务、异常传递 | 进行中，售票记录查询待完成 |

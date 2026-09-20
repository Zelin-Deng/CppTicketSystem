# C++ 多线程售票系统

使用 C++ 实现的售票学习项目，从类封装、互斥锁和请求队列，逐步演进到通用线程池和 HTTP API。

## 当前版本：V5 — Network API

```text
客户端 / Postman
    ↓ HTTP + JSON
ApiServer：校验请求、生成请求编号
    ↓ submitPurchaseRequest()
TicketSystem → ThreadPool → processOneRequest()
    ↓ promise / future 传回 PurchaseResult
ApiServer → HTTP 状态码 + JSON 响应
```

- `GET /health`：健康检查。
- `GET /tickets`：查询剩余票数。
- `POST /tickets/purchase`：提交购票请求，返回票号和剩余库存。
- 业务线程池固定为 3 个线程，库存、票号与销售记录由互斥锁保护。
- 服务器启动时初始化 100 张票；数据保存在内存中，重启后重置。

## 版本演进与历史编号

这里的 V1–V5 表示当前学习路线中的阶段。早期 GitHub 编号与路线编号不同；保留原有提交、分支名和标签，不重写历史。

| 当前路线阶段 | 功能 | 对应的已有 Git 历史 |
| --- | --- | --- |
| V1 — 基础多线程 | 学习线程与共享票数，是路线中的起点 | 未单独作为本仓库的首次提交 |
| V2 — 类封装与互斥锁 | `TicketSystem` 类、互斥保护、销售记录和窗口统计 | 首次提交 `6c52e77`，当时命名为 V1，标签 `v1.0` |
| V3 — Request Queue / Producer–Consumer | `TicketRequest`、请求队列、条件变量、工作线程消费请求 | `3145b96`，早期分支 `feature/v2-request-queue`、标签 `v2.0` |
| V4 — Thread Pool | 独立线程池、通用任务队列、等待任务完成和线程回收 | `f774eef`，分支 `feature/v4-thread-pool`、标签 `v4.0` |

**早期 `feature/v2-request-queue` 实际对应当前规划中的 V3 Request Queue 阶段。** 旧提交信息、README 或标签中的 V2 是当时的开发编号，不表示缺少请求队列阶段。

### V3 — 请求队列与生产者—消费者模型

用户提交购票请求，`TicketSystem` 将请求加入 `requestQueue`，通过 `condition_variable` 通知窗口线程领取并处理请求。该阶段在业务类中管理请求队列和线程协作，记录请求、用户及售票结果。

```text
用户请求 → TicketSystem::requestQueue → 窗口工作线程 → processOneRequest()
```

### V4 — 通用线程池

V4 把 V3 的线程调度职责移入 `ThreadPool`，使用 `queue<std::function<void()>>` 保存可调用任务。`TicketSystem` 将购票请求封装为任务提交给线程池，保留业务数据的互斥保护。

已发布的 `v4.0` 标签保留在源码提交 `f774eef`。V4 的 README 整理作为后续文档提交，不移动该标签；查看 `v4.0` 时会看到发布当时的旧 README。

### V5 — Network API

新增 `ApiServer`，通过 cpp-httplib 接收 HTTP 请求，使用 nlohmann/json 解析和生成 JSON。`TicketSystem::submitPurchaseRequest()` 返回 `future<PurchaseResult>`，HTTP 处理函数等待线程池中的业务任务完成后返回状态码和结果。对应分支为 `feature/v5-network-api`，版本标签为 `v5.0`。

## 项目结构

```text
CppTicketSystem.sln
TicketSystemCore/
├── main.cpp                    # 启动服务器
├── ApiServer.h / .cpp          # HTTP 路由与 JSON 转换
├── TicketSystem.h / .cpp       # 购票业务与异步结果
├── ThreadPool.h / .cpp         # 任务队列与工作线程
├── external/httplib.h          # cpp-httplib 0.56.0
├── external/json.hpp           # nlohmann/json 3.12.0
├── TicketSystemCore.vcxproj
└── TicketSystemCore.vcxproj.filters
```

两个第三方库均以头文件随仓库提供，文件中保留上游版权及许可证声明。

## 构建与运行

使用 Windows 10 或更新系统、Visual Studio 2022、MSVC v143 工具集和 Windows SDK，并安装“使用 C++ 的桌面开发”。

1. 打开 `CppTicketSystem.sln`。
2. 选择 `Debug | x64` 或 `Release | x64`，生成解决方案。
3. 使用 Ctrl+F5 运行 `TicketSystemCore`，保持服务器窗口开启。
4. 使用 Postman 或下方命令访问 `http://localhost:8080`。

也可在 Visual Studio Developer PowerShell 的仓库根目录执行：

```powershell
msbuild .\CppTicketSystem.sln /m /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\TicketSystemCore.exe
```

程序持续监听 `0.0.0.0:8080`；本机测试使用 `localhost:8080`，确保端口未被占用。使用 Ctrl+C 停止程序。

## API 使用示例

在另一个 PowerShell 窗口执行：

```powershell
Invoke-RestMethod http://localhost:8080/health
Invoke-RestMethod http://localhost:8080/tickets
Invoke-RestMethod http://localhost:8080/tickets/purchase -Method Post -ContentType 'application/json' -Body '{"user_id":1001,"ticket_count":2}'
```

初始余票查询返回 `{"remaining_tickets":100}`。首次购买 2 张票成功时返回 HTTP 201：

```json
{
  "success": true,
  "message": "Purchase successful",
  "ticket_ids": [1, 2],
  "remaining_tickets": 98
}
```

| 场景 | HTTP 状态码 | 说明 |
| --- | --- | --- |
| 健康检查、余票查询 | 200 | 返回服务器状态或剩余票数 |
| 购票成功 | 201 | 返回票号列表与剩余票数 |
| JSON 无法解析、缺少字段、字段不是整数 | 400 | 拒绝无效请求 |
| `ticket_count` 小于等于 0 | 400 | 拒绝无效购票数量 |
| 库存不足 | 409 | 不扣减库存 |
| 处理函数捕获到其他标准异常 | 500 | 返回内部错误 |

`user_id` 和 `ticket_count` 是必填整数字段；`requestId` 由服务器自动生成。并发请求的处理顺序不固定，返回票号也可能不同。

## 当前范围与后续改进

V5 是 HTTP 售票学习版本，尚未实现登录鉴权、持久化、退款或 `/statistics` 接口。当前未对整数超出 C++ `int` 范围的情况做显式校验，线程池也未提供任务异常捕获或工作线程数量校验。后续可完善参数边界、异常处理和数据库存储。

## 仓库文件管理

仅提交源码、第三方头文件、项目配置和文档。`.gitignore` 排除 `.vs/`、`x64/`、`x86/`、`Debug/`、`Release/` 以及 `*.obj`、`*.exe`、`*.pdb` 等构建产物。

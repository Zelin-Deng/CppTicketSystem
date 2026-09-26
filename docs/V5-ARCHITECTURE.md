# V5 架构图与执行流程

[返回 README](../README.md) · [API 说明](API.md)

本文对应已提交的 V5 Network API 实现（源码提交 `31ad72b`，标签 `v5.0`）。

## 组件架构

```mermaid
flowchart TB
    Main[main.cpp] -->|100 张票 / 3 个业务线程| TS[TicketSystem]
    Main -->|引用 TicketSystem / start 8080| API[ApiServer]
    Client[HTTP 客户端] <-->|HTTP + JSON| HTTP[cpp-httplib Server]
    API -->|注册路由 / 监听 0.0.0.0| HTTP
    HTTP --> Health[GET /health]
    HTTP --> Query[GET /tickets]
    HTTP --> Buy[POST /tickets/purchase]
    Buy --> JSON[nlohmann/json 解析与字段校验]
    JSON --> ID[atomic nextRequestId]
    ID -->|TicketRequest| TS
    TS -->|submit 闭包任务| Queue[ThreadPool tasks 队列]
    Queue -->|condition 唤醒| Workers[3 个业务工作线程]
    Workers --> Process[processOneRequest]
    Process -->|dataMutex| State[库存 / 连续票号 / SaleRecord / 计数器]
    Query -->|getRemainingTickets / dataMutex| State
    Process -->|set_value| Promise[promise PurchaseResult]
    Promise --> Future[future PurchaseResult]
    Future -->|get 等待结束| Response[ApiServer 状态码与 JSON 转换]
    Response --> HTTP
```

| 组件 | 职责 | 对应源码 |
| --- | --- | --- |
| `main` | 创建业务系统和 API 服务，指定库存、线程数、端口 | [main.cpp](../TicketSystemCore/main.cpp) |
| `ApiServer` | 注册路由、解析 JSON、校验字段、分配请求编号、映射 HTTP 状态码 | [ApiServer.h](../TicketSystemCore/ApiServer.h)、[ApiServer.cpp](../TicketSystemCore/ApiServer.cpp) |
| `TicketSystem` | 检查购票数量与库存，分配票号，维护销售记录与请求统计 | [TicketSystem.h](../TicketSystemCore/TicketSystem.h)、[TicketSystem.cpp](../TicketSystemCore/TicketSystem.cpp) |
| `ThreadPool` | 保存通用任务、唤醒线程、跟踪活动任务、等待完成和回收线程 | [ThreadPool.h](../TicketSystemCore/ThreadPool.h)、[ThreadPool.cpp](../TicketSystemCore/ThreadPool.cpp) |

## 一次购票的时序

```mermaid
sequenceDiagram
    participant C as 客户端
    participant A as ApiServer HTTP 处理线程
    participant T as TicketSystem
    participant P as ThreadPool
    participant W as 业务工作线程
    C->>A: POST /tickets/purchase
    A->>A: 解析 JSON / 检查必填整数字段
    alt JSON 或字段校验失败
        A-->>C: 400 + success / message
    else 字段校验通过
        A->>A: 原子递增 requestId
        A->>T: submitPurchaseRequest(request)
        T->>T: 创建共享 promise 并取得 future
        T->>P: submit(捕获 request 与 promise 的任务)
        T-->>A: future
        A->>A: future.get() 等待结果
        P->>W: 从队列取出任务
        W->>T: processOneRequest(request)
        Note over T: 持有 dataMutex：检查数量、库存并更新业务数据
        T-->>W: PurchaseResult
        W->>W: promise.set_value(result)
        Note over A,W: future 就绪，HTTP 处理线程继续
        A-->>C: 201 成功 / 409 库存不足 / 400 数量无效
    end
```

`GET /health` 直接返回固定 JSON；`GET /tickets` 加锁读取库存。这两个接口都不提交业务线程池任务。

## 并发与数据一致性

- **两层线程调度**：cpp-httplib 管理 HTTP 请求执行；自定义 `ThreadPool` 管理购票任务。`main` 中的 3 仅是业务工作线程数。
- **任务队列锁 `queueMutex`**：保护任务队列、`activeTasks` 和停止标志。工作线程取出任务后释放队列锁，再执行业务。
- **业务锁 `dataMutex`**：覆盖 `processOneRequest()` 的整个处理过程，保护库存、票号、销售记录和成功/失败计数。余票读取也使用此锁。
- **整单处理**：先检查数量大于 0，再检查库存足够；通过后一次性分配本单票号并扣减库存。数量无效或库存不足时不售出任何票。
- **请求编号**：`ApiServer::nextRequestId` 从 1 开始原子递增，字段校验通过后分配；不作为 HTTP 响应字段返回。
- **结果传递**：闭包按值捕获请求，用 `shared_ptr` 保持 `promise` 存活。调用方持有 `future`，通过 `get()` 获取 `PurchaseResult`。
- **顺序与快照**：任务按入队顺序出队，但多线程竞争业务锁，完成顺序不保证与请求编号一致。响应余票是该次业务处理时的快照。

## 生命周期与边界

服务器阻塞监听 `0.0.0.0:8080`。库存、票号、请求编号和销售记录只在当前进程内保存，重启后重新初始化。

正常调用线程池析构函数时，会先等待队列为空且 `activeTasks == 0`，再设置停止标志、唤醒并 `join` 工作线程。当前入口没有专门的 HTTP 优雅关闭流程，不能把强制终止进程视为完成了这一析构流程。

HTTP 层的异常捕获不覆盖业务工作线程中的异常；当前线程池未捕获任务异常，也未通过 `promise.set_exception()` 传递异常。`submitPurchaseRequest()` 未检查线程池 `submit()` 的布尔结果。上述属于当前实现边界。

V5 尚无数据库、登录鉴权、退款、HTTP 统计查询、请求幂等去重或业务队列容量限制。`printStatistics()` 和 `printSaleRecords()` 是 C++ 方法，未暴露为 HTTP 接口。

# V5 HTTP API 说明

[返回 README](../README.md) · [V5 架构图](V5-ARCHITECTURE.md)

本文对应已提交的 V5 源码 `31ad72b`（标签 `v5.0`）。接口行为以该版本的 `ApiServer.cpp` 和 `TicketSystem.cpp` 为准。

## 基本约定

- 本机地址：`http://localhost:8080`，服务监听 `0.0.0.0:8080`。
- 请求体使用 JSON，购票时发送 `Content-Type: application/json`；下述路由的应用响应类型为 `application/json`。
- 当前无鉴权。初始库存为 100，票号从 1 开始，重启服务后重置。
- 字段名区分大小写；额外字段不参与处理。未定义路由的响应由 HTTP 库处理，不保证使用本文的 JSON 错误格式。

| 方法 | 路径 | 用途 | 正常状态码 |
| --- | --- | --- | --- |
| GET | `/health` | 健康检查 | 200 |
| GET | `/tickets` | 查询剩余票数 | 200 |
| POST | `/tickets/purchase` | 购买指定数量的票 | 201 |

## GET /health

无请求参数。返回 HTTP 200：

```json
{
  "status": "ok",
  "message": "Ticket Server is running"
}
```

此接口返回固定状态，不检查业务队列或外部依赖。

## GET /tickets

无请求参数。新启动且尚未售票时返回 HTTP 200：

```json
{
  "remaining_tickets": 100
}
```

`remaining_tickets` 为读取时的整数库存快照，不预留票额。

## POST /tickets/purchase

### 请求

```json
{
  "user_id": 1001,
  "ticket_count": 2
}
```

| 字段 | 类型 | 必填 | 当前规则 |
| --- | --- | --- | --- |
| `user_id` | JSON 整数 | 是 | 写入销售记录；当前不校验用户是否存在，也不限制为正数 |
| `ticket_count` | JSON 整数 | 是 | 必须大于 0，且不能超过处理时的余票 |

字符串、浮点数（包括 `2.0`）、布尔值和 `null` 不满足整数检查。客户端应将两字段限制在 MSVC 32 位 `int` 范围 `[-2147483648, 2147483647]` 内；当前服务未显式校验超出此范围的整数，不保证将其返回为 400。

服务器在字段校验通过后生成内部 `requestId`，客户端无需提供。它不出现在响应中，也不能用于去重。

### 成功：HTTP 201

初始库存 100 时首次购买 2 张票：

```json
{
  "success": true,
  "message": "Purchase successful",
  "ticket_ids": [1, 2],
  "remaining_tickets": 98
}
```

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `success` | boolean | 本次购票是否成功 |
| `message` | string | 处理结果文本 |
| `ticket_ids` | integer[] | 本次分配的票号，业务失败时为空数组 |
| `remaining_tickets` | integer | 本次处理时的剩余库存 |

单次购票不会因库存不足而部分成功。并发调用时，票号与余票可能不同于示例。

### 参数或解析错误：HTTP 400

JSON 解析、必填字段或字段类型检查失败时，仅返回 `success` 和 `message`：

```json
{
  "success": false,
  "message": "Missing required fields"
}
```

| 触发条件 | 精确的 `message` |
| --- | --- |
| JSON 无法解析（包括空请求体） | `Invalid JSON format` |
| 缺少 `user_id` 或 `ticket_count` | `Missing required fields` |
| 两字段任意一个不是 JSON 整数 | `user_id and ticket_count must be integers` |

校验按解析、字段存在、字段类型的顺序进行。以上错误消息末尾没有句号。

### 数量无效：HTTP 400

`ticket_count <= 0` 在业务层被拒绝，会返回全部四个结果字段；以下假设余票为 100：

```json
{
  "success": false,
  "message": "Invalid ticket count",
  "ticket_ids": [],
  "remaining_tickets": 100
}
```

### 库存不足：HTTP 409

请求数量超过处理时的余票时，不扣减库存。以下假设余票为 1：

```json
{
  "success": false,
  "message": "Not enough tickets",
  "ticket_ids": [],
  "remaining_tickets": 1
}
```

### 内部错误：HTTP 500

购票 HTTP 处理函数捕获到除 JSON 解析错误以外的标准异常时返回：

```json
{
  "success": false,
  "message": "Internal server error"
}
```

这一捕获范围不包含业务工作线程中逃逸的异常，不能保证所有内部故障都返回此 JSON。

## PowerShell 调用示例

先按 README 启动服务，再在另一个 PowerShell 窗口运行：

```powershell
Invoke-RestMethod -Uri 'http://localhost:8080/health'
Invoke-RestMethod -Uri 'http://localhost:8080/tickets'
Invoke-RestMethod -Uri 'http://localhost:8080/tickets/purchase' `
    -Method Post `
    -ContentType 'application/json' `
    -Body '{"user_id":1001,"ticket_count":2}'
```

查看错误状态码及响应体可用 `curl.exe`。下面通过标准输入传 JSON，避免 Windows 命令行的双引号转义差异：

```powershell
'{"user_id":1001,"ticket_count":0}' | curl.exe -i -X POST 'http://localhost:8080/tickets/purchase' -H 'Content-Type: application/json' --data-binary '@-'
```

Postman 中选择对应方法和 URL；购票接口选择 **Body → raw → JSON**，粘贴请求体并发送。

## 手工验证清单

使用刚启动的服务，在没有其他客户端购票的情况下依次执行。购票行均发送至 `POST /tickets/purchase`：

| 步骤 | 请求 | 预期状态码 | 预期结果 |
| --- | --- | --- | --- |
| 1 | `GET /health` | 200 | `status` 为 `ok` |
| 2 | `GET /tickets` | 200 | 余票 100 |
| 3 | `{"user_id":1001,"ticket_count":2}` | 201 | 票号 `[1,2]`，余票 98 |
| 4 | `{"user_id":1001}` | 400 | `Missing required fields` |
| 5 | `{"user_id":1001,"ticket_count":"2"}` | 400 | 字段类型错误 |
| 6 | 原始请求体 `{` | 400 | `Invalid JSON format` |
| 7 | `{"user_id":1001,"ticket_count":0}` | 400 | 空票号数组，余票 98 |
| 8 | `{"user_id":1001,"ticket_count":99}` | 409 | 空票号数组，余票 98 |
| 9 | `GET /tickets` | 200 | 余票仍为 98 |

## 调用边界

当前没有幂等键或重复请求检测，同一请求重复发送会再次购买。客户端未收到响应并不表示购票失败，自动重试可能造成重复购票。

业务线程池异步处理任务，但 HTTP 处理函数通过 `future.get()` 等待业务完成后才返回；当前没有设置业务结果等待超时。库存快照不能保证下一次购票仍然有票。

当前没有用户查询、订单查询、退款或 `/statistics` HTTP 接口。

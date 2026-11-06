# V7 架构安排

## 目标

保持一个 C++ 程序，采用轻量分层、Repository 接口和构造函数注入，使业务代码更容易测试，也方便替换数据库实现。

V6 已完成数据持久化。V7 重点改善代码之间的依赖关系，用户购票记录查询放在后续登录功能阶段。

## 当前问题

- `TicketSystem` 已通过构造函数接收数据库对象，但依赖的是具体的 `TicketRepository`。
- `InventoryData` 定义在具体数据库类的头文件中。
- 数据库路径固定为 `ticket.db`，不方便指定独立测试数据库。
- 业务测试需要真实 SQLite；购票业务与线程池调度也放在同一个类中。

## 第一阶段：数据库解耦

| 顺序 | 修改 | 目的 |
| --- | --- | --- |
| 1 | 将 `InventoryData` 提取为独立的数据类型 | 数据对象与具体数据库实现分离 |
| 2 | 新增 `ITicketRepository`，声明 `getInventory()`、`savePurchase()` 和虚析构函数 | 定义业务实际需要的存取约定 |
| 3 | 将现有数据库类改为实现接口的 `SqliteTicketRepository` | 保留现有 SQL 和事务逻辑 |
| 4 | 将 `TicketSystem` 中的依赖改为 `ITicketRepository&` | 业务通过接口调用数据操作 |
| 5 | 在 `main()` 创建 SQLite 实现、初始化数据库，再注入业务类 | 集中选择和组合具体实现 |
| 6 | 通过构造函数传入数据库路径 | 可以独立指定测试数据库 |
| 7 | 增加测试用 `MemoryTicketRepository` | 不打开 SQLite 也能验证业务规则 |

接口使用普通 C++ 数据类型，不暴露 SQL、`sqlite3*` 或 SQLite 错误码。`initDatabase()` 留在具体实现中，由启动代码调用。查询记录等功能在真正需要时再扩展接口。

依赖关系：

```text
ApiServer → TicketSystem → ITicketRepository
                              ↑ 实现接口
                  SqliteTicketRepository
                  MemoryTicketRepository（测试用）
```

普通数据类型放在 domain，业务类和仓库接口放在 application，HTTP 代码放在 presentation，SQLite 实现放在 infrastructure。先调整依赖，再整理目录。

## 必须保留的行为

- `savePurchase()` 成功时，库存与本次售票记录全部保存；失败时不留下部分修改。
- 事务由数据库实现内部管理，业务层不执行 BEGIN、COMMIT 或 ROLLBACK。
- 数据库保存成功后才更新业务对象的内存状态。
- 保留现有 HTTP 路由、JSON 字段和状态码。
- 当前仍面向单个服务实例；抽象接口本身不解决多个服务同时修改库存的问题。

## 第二阶段：按需要分离任务调度

第一阶段通过测试后，再将同步购票逻辑与线程池任务提交、promise/future 传递分开，使业务规则能独立测试。届时按实际需要确定调度接口，不提前给每个类增加抽象层。

## 验收方式

- 内存仓库：测试正常购票、数量无效、库存不足，以及模拟保存失败后业务状态不变。
- SQLite 实现：验证正常保存、重启恢复，以及第二条售票记录失败时整笔事务回滚。
- HTTP 接口：确认原有请求和响应仍然可用。
- 对照依赖：购票业务不包含 SQLite 实现头文件，切换仓库实现时不修改业务算法。

内存测试不能替代真实数据库事务测试。更换数据库仍需实现对应驱动、SQL、表结构和数据迁移，但这些改动应集中在数据库实现与启动配置中。

## 对应课程知识

参考 Task2 课程材料：

- Lesson 8-1 DI and DIP：构造函数注入、依赖倒置、SQLite 适配器示例。
- Lesson 8-2 Architectural concepts：职责分层、业务与基础设施分离、Repository Pattern。
- Task 2 Assessment Brief：业务可独立测试、通过接口访问存储、使用依赖注入。

借鉴课程的设计原则，继续使用 C++ 和现有 HTTP、SQLite 库。保持个人学习项目的规模，不引入微服务、复杂 DI 容器或不需要的设计模式。

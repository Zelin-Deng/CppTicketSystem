# C++ 多线程售票系统

使用 C++ 实现的多线程售票学习项目，逐步实践类封装、互斥锁、请求队列、生产者—消费者模型和通用线程池。

## 当前版本：V4 — Thread Pool

V4 在 V3 请求队列阶段的基础上，将任务队列和工作线程管理抽离到独立的 `ThreadPool`。`TicketSystem` 专注于库存校验、票号生成、销售记录和统计。

```text
main.cpp 构造 TicketRequest
    ↓
TicketSystem::submitRequest()
    ↓
ThreadPool::submit() → queue<std::function<void()>>
    ↓ condition_variable 唤醒工作线程
ThreadPool::workerLoop()
    ↓
TicketSystem::processOneRequest()
    ↓ dataMutex 保护业务数据
校验数量与库存 → 生成票号 → 扣减库存 → 保存记录 → 输出结果
```

- 固定数量的工作线程处理通用任务，任务从队列取出后在队列锁之外执行。
- `waitUntilFinished()` 等待任务队列为空且正在执行的任务数为零。
- 线程池析构时等待已提交任务完成，唤醒并回收工作线程。
- 购票数量必须大于零且不超过剩余库存；失败请求计入失败统计。
- `submitRequest()` 的返回值表示任务是否被线程池接收，不代表购票成功；业务结果由任务处理并输出。

## 版本演进与历史编号

这里的 V1–V4 表示当前学习路线中的阶段。早期 GitHub 编号与路线编号不同；保留原有提交、分支名和标签，不重写历史。

| 当前路线阶段 | 功能 | 对应的已有 Git 历史 |
| --- | --- | --- |
| V1 — 基础多线程 | 学习线程与共享票数，是路线中的起点 | 未单独作为本仓库的首次提交 |
| V2 — 类封装与互斥锁 | `TicketSystem` 类、互斥保护、销售记录和窗口统计 | 首次提交 `6c52e77`，当时命名为 V1 |
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

已发布的 `v4.0` 标签保留在源码提交 `f774eef`。本次 README 整理作为后续文档提交，不移动该标签；查看 `v4.0` 时会看到发布当时的旧 README。

## 项目结构

```text
CppTicketSystem.sln
TicketSystemCore/
├── main.cpp                       # 演示入口
├── TicketSystem.h / .cpp           # 购票请求、业务处理与统计
├── ThreadPool.h / .cpp             # 通用任务队列与工作线程
├── TicketSystemCore.vcxproj        # Visual Studio 项目
└── TicketSystemCore.vcxproj.filters
```

## 构建与运行

使用 Visual Studio 2022，安装“使用 C++ 的桌面开发”、MSVC v143 工具集和 Windows SDK。

1. 打开 `CppTicketSystem.sln`。
2. 选择 `Debug | x64` 或 `Release | x64`，生成解决方案。
3. 运行 `TicketSystemCore`（可使用 Ctrl+F5）。

也可在 Visual Studio Developer PowerShell 中从仓库根目录执行：

```powershell
msbuild .\CppTicketSystem.sln /m /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\TicketSystemCore.exe
```

当前示例配置 100 张票、3 个工作线程，提交 7 个请求：6 个有效请求共购买 17 张票，1 个请求因数量为 0 被拒绝。完成后应剩余 83 张票，成功请求 6 个、失败请求 1 个，销售记录 17 条。并发执行时，请求处理顺序和票号与请求的对应关系可能不同。

当前是单机控制台学习版本。调用方应提供正数工作线程数量，提交有效且不抛出异常的任务，并在销毁线程池前停止提交；当前实现未提供任务异常捕获或构造参数校验。

## 后续规划

V5 计划增加网络/API 层，让外部客户端通过 HTTP 调用售票业务；当前 V4 尚未实现网络接口。

## 仓库文件管理

仅提交源码、项目配置和文档。`.gitignore` 排除 `.vs/`、`x64/`、`x86/`、`Debug/`、`Release/` 以及 `*.obj`、`*.exe`、`*.pdb` 等构建产物。

#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <future>
#include <memory>

#include "ThreadPool.h"
#include "TicketRepository.h"
#include "SaleRecord.h"

using namespace std;

struct TicketRequest
{
    int requestId;
    int userId;
    int ticketCount;
};

struct PurchaseResult
{
    bool success;
    string message;
    vector<int> ticketIds;
    int remainingTickets;
};

class TicketSystem
{
private:

    // 数据库访问层
    TicketRepository& ticketRepository;

    // ===== 售票业务数据 =====
    int remainingTickets;
    int nextTicketId;


    // ===== 保护售票业务数据 =====
    mutex dataMutex;

    // ===== 销售记录 =====
    vector<SaleRecord> saleRecords;


    // ===== 通用线程池 =====
    ThreadPool threadPool;


public:

    TicketSystem(
        TicketRepository& repository,
        int workerCount
    );

    int getRemainingTickets();

    future<PurchaseResult> submitPurchaseRequest(
        const TicketRequest& request
    );

private:

    // 真正的售票业务逻辑
    PurchaseResult processOneRequest(const TicketRequest& request);
};
#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <future>
#include <memory>

#include "ThreadPool.h"

using namespace std;


struct SaleRecord
{
    int ticketId;
    int userId;
    int requestId;

    chrono::system_clock::time_point saleTime;
};


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
        int ticketCount,
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
#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>

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


class TicketSystem
{
private:

    // ===== 售票业务数据 =====
    int totalTickets;
    int remainingTickets;
    int nextTicketId;


    // ===== 统计 =====
    int successRequestCount;
    int failedRequestCount;


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


    // 提交一个售票请求
    bool submitRequest(
        const TicketRequest& request
    );

    int getRemainingTickets();

    void printSaleRecords();

    void printStatistics();

    void waitUntilFinished();

private:

    // 真正的售票业务逻辑
    void processOneRequest(const TicketRequest& request);
};
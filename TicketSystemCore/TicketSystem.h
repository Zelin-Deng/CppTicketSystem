#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>

//v2版本新增
#include <queue>
#include <condition_variable>

using namespace std;

struct SaleRecord
{
	int ticketId;
	int windowId;
	int userId;
	int requestId;

	chrono::system_clock::time_point saleTime;
};

//v2新增结构
struct TicketRequest
{
	int requestId;
	int userId;
	int ticketCount;
};


class TicketSystem
{
private:
	int totalTickets;
	int remainingTickets;
	int nextTicketId;

	int successRequestCount;
	int failedRequestCount;

	mutex dataMutex;

	vector<SaleRecord> saleRecords;
	vector<int> windowSales;

	//v2新增成员
	queue<TicketRequest> requestQueue; //请求队列
	mutex queueMutex; //队列互斥锁
	condition_variable requestCondition; //条件变量对象 负责等待和唤醒售票线程
	bool acceptingRequests; //表示系统是否还会继续接收请求

public:
	TicketSystem(int ticketCount, int windowCount);

	int getRemainingTickets();

	void printSaleRecords();

	void printStatistics();

	//v3新增接口
	bool submitRequest(const TicketRequest& request);

	void processRequests(int windowId);

	void stopAcceptingRequest();

private:
	void processOneRequest(const TicketRequest& request, int windowId);

};
#include "TicketSystem.h"

TicketSystem::TicketSystem(int ticketCount, int windowCount)
	: totalTickets(ticketCount),
	  remainingTickets(ticketCount),
	  nextTicketId(1),
	  acceptingRequests(true),
	  windowSales(windowCount,0),
	  failedRequestCount(0),
	  successRequestCount(0) 
{}

int TicketSystem::getRemainingTickets()
{
	lock_guard<mutex> lock(this->dataMutex);

	return this->remainingTickets;
}

void TicketSystem::printSaleRecords()
{
	lock_guard<mutex> lock(dataMutex);

	cout << "\n===== 售票记录 =====" << endl;

	for (const SaleRecord& record : saleRecords)
	{
		cout << "票号：" << record.ticketId << "，请求：" << record.requestId << "，窗口：" << record.windowId << endl;
	}
}

void TicketSystem::printStatistics()
{
	lock_guard<std::mutex> lock(dataMutex);

	cout << "\n===== V3售票统计 =====" << endl;

	cout << "总票数：" << totalTickets << endl;

	cout << "已售票数：" << saleRecords.size() << endl;

	cout << "剩余票数：" << remainingTickets << endl;

	cout << "成功请求数：" << successRequestCount << endl;

	cout << "失败请求数：" << failedRequestCount << endl;

	for (int i = 0; i < static_cast<int>(windowSales.size()); i++)
	{
		cout << "窗口" << i + 1 << "共售出" << windowSales[i] << "张票" << endl;
	}
}

bool TicketSystem::submitRequest(const TicketRequest& request)
{
	{
		lock_guard<mutex> lock(queueMutex);

		if (!acceptingRequests)
		{
			return false;
		}

		requestQueue.push(request);
	}

	requestCondition.notify_one();

	return true;
}

void TicketSystem::processRequests(int windowId)
{
	while (true)
	{
		TicketRequest request;

		{
			unique_lock<mutex> lock(queueMutex);

			requestCondition.wait(lock, [this]() 
				                     {
					                    return !requestQueue.empty() || !acceptingRequests; 
				                     });

			if (requestQueue.empty() && !acceptingRequests)
			{
				break;
			}

			request = requestQueue.front();
			requestQueue.pop();
		}

		processOneRequest(request, windowId);

		this_thread::sleep_for(chrono::milliseconds(50));
	}

	cout << "窗口" << windowId << "停止处理请求" << endl;
}

void TicketSystem::processOneRequest(const TicketRequest& request, int windowId)
{
	lock_guard<mutex> lock(dataMutex);

	if (windowId<1 || windowId>static_cast<int>(windowSales.size()))
	{
		failedRequestCount++;

		cout << "请求" << request.requestId << "处理失败：窗口编号非法" << endl;

		return;
	}

	if (request.ticketCount <= 0)
	{
		failedRequestCount++;

		cout << "请求" << request.requestId << "处理失败：购票数量必须大于0" << endl;

		return;
	}

	if (remainingTickets < request.ticketCount)
	{
		failedRequestCount++;

		cout << "窗口" << windowId << "处理用户" << request.userId << "的请求失败：余票不足" << "，申请" << request.ticketCount << "张，当前剩余" << remainingTickets << "张" << endl;

		return;
	}

	vector<int> soldTicketIds;

	for (int i = 0; i < request.ticketCount; i++)
	{
		const int soldTicketId = nextTicketId++;

		SaleRecord record;
		record.ticketId = soldTicketId;
		record.requestId = request.requestId;
		record.userId = request.userId;
		record.windowId = windowId;
		record.saleTime = chrono::system_clock::now();

		saleRecords.push_back(record);
		soldTicketIds.push_back(soldTicketId);
	}

	remainingTickets -= request.ticketCount;
	windowSales[windowId - 1] += request.ticketCount;

	successRequestCount++;

	cout << "窗口" << windowId << "成功处理请求" << request.requestId << "：用户" << request.userId << "购买" << request.ticketCount << "张票，票号：";

	for (int ticketId : soldTicketIds)
	{
		cout << ticketId << " ";
	}

	cout << "，剩余票数：" << remainingTickets << endl;
}

void TicketSystem::stopAcceptingRequest()
{
	{
		lock_guard<mutex> lock(queueMutex);

		acceptingRequests = false;
	}

	this->requestCondition.notify_all();
}


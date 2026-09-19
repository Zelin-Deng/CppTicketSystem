#include "TicketSystem.h"

TicketSystem::TicketSystem(int ticketCount,int workerCount)
	: totalTickets(ticketCount),
	remainingTickets(ticketCount),
	nextTicketId(1),
	successRequestCount(0),
	failedRequestCount(0),
	threadPool(workerCount)
{}

bool TicketSystem::submitRequest(const TicketRequest& request) {

	return threadPool.submit(
		[this, request]()
		{
			processOneRequest(request);
		}
	);
}

void TicketSystem::processOneRequest(const TicketRequest& request) {

	lock_guard<mutex> lock(dataMutex);

	 
	if (request.ticketCount <= 0)
	{
		failedRequestCount++;

		cout << "[FAILED ] "
			<< "Request=" << request.requestId
			<< " | User=" << request.userId
			<< " | Reason=Invalid ticket count"
			<< endl;

		return;
	}

	if (remainingTickets < request.ticketCount)
	{
		failedRequestCount++;

		cout << "[FAILED ] "
			<< "Request=" << request.requestId
			<< " | User=" << request.userId
			<< " | Count=" << request.ticketCount
			<< " | Reason=Not enough tickets"
			<< " | Remaining=" << remainingTickets
			<< endl;

		return;
	}
 
	vector<int> soldTicketIds;

	for (int i = 0; i < request.ticketCount; i++) {

		const int soldTicketId = nextTicketId++;

		SaleRecord record;
		record.ticketId = soldTicketId;
		record.requestId = request.requestId;
		record.userId = request.userId;
		record.saleTime = chrono::system_clock::now();

		saleRecords.push_back(record);
		soldTicketIds.push_back(soldTicketId);
	}

	remainingTickets -= request.ticketCount;

	successRequestCount++;

	cout << "[SUCCESS] "
		<< "Request=" << request.requestId
		<< " | User=" << request.userId
		<< " | Count=" << request.ticketCount
		<< " | Tickets=[";

	for (size_t i = 0; i < soldTicketIds.size(); i++)
	{
		cout << soldTicketIds[i];

		if (i != soldTicketIds.size() - 1)
		{
			cout << ", ";
		}
	}

	cout << "]"
		<< " | Remaining=" << remainingTickets
		<< endl;
}

int TicketSystem::getRemainingTickets() {

	lock_guard<mutex> lock(this->dataMutex);

	return this->remainingTickets;
}

void TicketSystem::printSaleRecords() {

	lock_guard<mutex> lock(dataMutex);

	cout << "\n===== 售票记录 =====" << endl;

	for (const SaleRecord& record : saleRecords) {

		cout << "票号：" << record.ticketId << "，请求：" << record.requestId << "，用户：" << record.userId << endl;
	}
}

void TicketSystem::printStatistics() {

	lock_guard<std::mutex> lock(dataMutex);

	cout << "\n===== V4售票统计 =====" << endl;

	cout << "总票数：" << totalTickets << endl;

	cout << "已售票数：" << saleRecords.size() << endl;

	cout << "剩余票数：" << remainingTickets << endl;

	cout << "成功请求数：" << successRequestCount << endl;

	cout << "失败请求数：" << failedRequestCount << endl;
}

void TicketSystem::waitUntilFinished()
{
	threadPool.waitUntilFinished();
}
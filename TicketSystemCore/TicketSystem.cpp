#include "TicketSystem.h"

TicketSystem::TicketSystem(int ticketCount,int workerCount)
	: totalTickets(ticketCount),
	remainingTickets(ticketCount),
	nextTicketId(1),
	successRequestCount(0),
	failedRequestCount(0),
	threadPool(workerCount)
{}

PurchaseResult TicketSystem::processOneRequest(const TicketRequest& request) {

    lock_guard<mutex> lock(dataMutex);

    PurchaseResult result;

    // 1. 检查购票数量
    if (request.ticketCount <= 0)
    {
        failedRequestCount++;

        result.success = false;
        result.message = "Invalid ticket count";
        result.remainingTickets = remainingTickets;

        return result;
    }

    // 2. 检查余票
    if (remainingTickets < request.ticketCount)
    {
        failedRequestCount++;

        result.success = false;
        result.message = "Not enough tickets";
        result.remainingTickets = remainingTickets;

        return result;
    }

    // 3. 真正售票
    for (int i = 0; i < request.ticketCount; i++)
    {
        int ticketId = nextTicketId++;

        result.ticketIds.push_back(ticketId);

        SaleRecord record;

        record.ticketId = ticketId;
        record.userId = request.userId;
        record.requestId = request.requestId;
        record.saleTime =
            chrono::system_clock::now();

        saleRecords.push_back(record);
    }

    remainingTickets -= request.ticketCount;
    successRequestCount++;

    // 4. 返回业务结果
    result.success = true;
    result.message = "Purchase successful";
    result.remainingTickets = remainingTickets;

    return result;
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

future<PurchaseResult> TicketSystem::submitPurchaseRequest(const TicketRequest& request) {

    auto resultPromise =
        make_shared<promise<PurchaseResult>>();

    future<PurchaseResult> resultFuture =
        resultPromise->get_future();

    threadPool.submit(
        [this, request, resultPromise]()
        {
            PurchaseResult result =
                processOneRequest(request);

            resultPromise->set_value(result);
        }
    );

    return resultFuture;
}
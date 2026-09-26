#include "TicketSystem.h"

TicketSystem::TicketSystem(int ticketCount,int workerCount)
	: remainingTickets(ticketCount),
	nextTicketId(1),
	threadPool(workerCount)
{}

PurchaseResult TicketSystem::processOneRequest(const TicketRequest& request) {

    lock_guard<mutex> lock(dataMutex);

    PurchaseResult result;

    // 1. 检查购票数量
    if (request.ticketCount <= 0)
    {
        result.success = false;
        result.message = "Invalid ticket count";
        result.remainingTickets = remainingTickets;

        return result;
    }

    // 2. 检查余票
    if (remainingTickets < request.ticketCount)
    {
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
        record.saleTime = chrono::system_clock::now();

        saleRecords.push_back(record);
    }

    remainingTickets -= request.ticketCount;

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

future<PurchaseResult> TicketSystem::submitPurchaseRequest(const TicketRequest& request) {

    auto resultPromise = make_shared<promise<PurchaseResult>>();

    future<PurchaseResult> resultFuture = resultPromise->get_future();

    threadPool.submit(
        [this, request, resultPromise]()
        {
            PurchaseResult result = processOneRequest(request);

            resultPromise->set_value(result);
        }
    );

    return resultFuture;
}
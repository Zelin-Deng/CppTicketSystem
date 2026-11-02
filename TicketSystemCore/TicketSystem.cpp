#include "TicketSystem.h"
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

TicketSystem::TicketSystem(TicketRepository& repository, int workerCount)
	: ticketRepository(repository),
    remainingTickets(0),
	nextTicketId(1),
	threadPool(workerCount)
{
    // 从数据库读取已保存的库存状态
    InventoryData inventory = ticketRepository.getInventory();

    // 初始化业务核心层的内存状态
    remainingTickets = inventory.remainingTickets;
    nextTicketId = inventory.nextTicketId;
}

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

    // 3. 确保计算新票号时不会超出 int 范围。
    if (nextTicketId < 1 || request.ticketCount >
        (std::numeric_limits<int>::max)() - nextTicketId)
    {
        throw std::overflow_error("Ticket ID limit reached");
    }

    // 4. 在临时变量中准备新状态，数据库保存前不修改成员变量。
    int newRemainingTickets = remainingTickets - request.ticketCount;
    int newNextTicketId = nextTicketId;
    vector<SaleRecord> newSaleRecords = saleRecords;

    // 只收集本次购买的记录，避免把历史记录再次插入数据库。
    vector<SaleRecord> purchaseRecords;

    for (int i = 0; i < request.ticketCount; i++)
    {
        int ticketId = newNextTicketId++;
        result.ticketIds.push_back(ticketId);

        SaleRecord record;
        record.ticketId = ticketId;
        record.userId = request.userId;
        record.requestId = request.requestId;
        record.saleTime = chrono::system_clock::now();
        newSaleRecords.push_back(record);
        purchaseRecords.push_back(record);
    }

    // 字符串及容器的准备都在写数据库之前完成。
    result.success = true;
    result.message = "Purchase successful";
    result.remainingTickets = newRemainingTickets;

    // 5. 在同一事务中保存库存及本次记录；失败回滚，内存保持原值。
    ticketRepository.savePurchase(
        newRemainingTickets, newNextTicketId, purchaseRecords
    );

    // 6. 数据库保存成功后，才提交内存状态。
    remainingTickets = newRemainingTickets;
    nextTicketId = newNextTicketId;
    saleRecords.swap(newSaleRecords);

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
            try
            {
                PurchaseResult result = processOneRequest(request);

                // 成功：把购买结果交给 HTTP 请求线程。
                resultPromise->set_value(std::move(result));
            }
            catch (...)
            {
                // 失败：把异常交给 HTTP 请求线程。
                resultPromise->set_exception(
                    std::current_exception()
                );
            }
        }
    );

    return resultFuture;
}
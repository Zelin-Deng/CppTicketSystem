#include "TicketSystem.h"

TicketSystem::TicketSystem(int ticketCount, int windowCount)
	: totalTickets(ticketCount),
	  remainingTickets(ticketCount),
	  nextTicketId(1),
	  windowSales(windowCount,0)
{}

bool TicketSystem::sellOneTicket(int windowId)
{
	lock_guard<mutex> lock(this->dataMutex);

	if (this->remainingTickets <= 0)
	{
		return false;
	}

	int soldTicketId = this->nextTicketId;
	this->nextTicketId++;
	this->remainingTickets--;

	SaleRecord record;
	record.ticketId = soldTicketId;
	record.windowId = windowId;
	record.saleTime = chrono::system_clock::now();

	this->saleRecords.push_back(record);
	this->windowSales[windowId - 1]++;

	cout << "窗口" << windowId << "卖出票号" << soldTicketId << "，剩余票数：" << remainingTickets << endl;

	return true;
}

int TicketSystem::getRemainingTickets()
{
	lock_guard<mutex> lock(this->dataMutex);

	return this->remainingTickets;
}

void TicketSystem::printSaleRecords()
{

}

void TicketSystem::printStatistics()
{
	lock_guard<mutex> lock(dataMutex);

	cout << "\n===== 售票统计 =====" << endl;

	cout << "总票数：" << totalTickets << endl;
	cout << "已售票数：" << saleRecords.size() << endl;
	cout << "剩余票数：" << remainingTickets << endl;

	for (int i = 0; i < this->windowSales.size(); i++)
	{
		cout << "窗口" << i + 1
			<< "共售出" << this->windowSales[i]
			<< "张票"
			<< endl;
	}
}
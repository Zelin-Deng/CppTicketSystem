#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>

using namespace std;

struct SaleRecord
{
	int ticketId;
	int windowId;
	chrono::system_clock::time_point saleTime;
};


class TicketSystem
{
private:
	int totalTickets;
	int remainingTickets;
	int nextTicketId;

	mutex dataMutex;

	vector<SaleRecord> saleRecords;
	vector<int> windowSales;

public:
	TicketSystem(int ticketCount, int windowCount);

	bool sellOneTicket(int windowId);

	int getRemainingTickets();

	void printSaleRecords();

	void printStatistics();
};
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include "TicketSystem.h"

using namespace std;

void windowWorker(TicketSystem& system, int windowId)
{
	while (system.sellOneTicket(windowId))
	{
		this_thread::sleep_for(chrono::milliseconds(50));
	}

	cout << "´°¿Ú" << windowId << "Í£Ö¹ÊÛÆ±" << endl;
}

int main()
{
	const int totalCount = 100;
	const int windowsCount = 3;

	TicketSystem system(totalCount, windowsCount);

	vector<thread> windows;

	for (int i = 1; i <= windowsCount; i++)
	{
		windows.emplace_back(windowWorker, ref(system), i);
	}
	
	for (auto& window : windows)
	{
		if (window.joinable())
		{
			window.join();
		}
	}

	system.printStatistics();

	return 0;
}

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include "TicketSystem.h"

using namespace std;

void windowWorker(TicketSystem& system, int windowId)
{
	system.processRequests(windowId);
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

	// main模拟不同用户
	vector<TicketRequest> requests =
	{
		{1, 1001, 2},
		{2, 1002, 1},
		{3, 1003, 3},
		{4, 1004, 4},
		{5, 1005, 2},
		{6, 1006, 5},
		{7, 1007, 0}
	};

	//提交请求入队
	for (const auto& request : requests)
	{
		const bool submitted = system.submitRequest(request);

		if (!submitted)
		{
			cout << "请求" << request.requestId << "提交失败：系统已关闭" << endl;
		}

		this_thread::sleep_for(chrono::milliseconds(30));
	}

	system.stopAcceptingRequest();
	
	for (auto& window : windows)
	{
		if (window.joinable())
		{
			window.join();
		}
	}

	system.printStatistics();
	system.printSaleRecords();

	return 0;
}

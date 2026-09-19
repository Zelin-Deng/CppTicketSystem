#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include "TicketSystem.h"

using namespace std;
 
int main() {

	const int totalCount = 100;
	const int workerCount = 3;

	TicketSystem system(totalCount, workerCount);

	vector<thread> windows;
 
	//模拟用户
	vector<TicketRequest> requests = {

		{1, 1001, 2},
		{2, 1002, 1},
		{3, 1003, 3},
		{4, 1004, 4},
		{5, 1005, 2},
		{6, 1006, 5},
		{7, 1007, 0}
	};

	//提交请求入队
	for (const auto& request : requests) {

		system.submitRequest(request);
	}

	system.waitUntilFinished();
	cout << endl;

	system.printStatistics();
	system.printSaleRecords();

	return 0;
}

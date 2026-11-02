#include <iostream>
#include <exception>

#include "ApiServer.h"
#include "TicketRepository.h"

using namespace std;

int main()
{
    cout << "========== Ticket System ==========" << endl;

    try
    {
        TicketRepository repository;
        repository.initDatabase();

        // 构造函数内部从数据库加载库存。
        TicketSystem ticketSystem(repository, 3);

        cout << "Loaded remaining tickets: "
            << ticketSystem.getRemainingTickets() << endl;

        ApiServer server(ticketSystem);
        server.start(8080);
    }
    catch (const std::exception& e)
    {
        cerr << "Application failed: "
            << e.what() << endl;

        return 1;
    }

    return 0;
}
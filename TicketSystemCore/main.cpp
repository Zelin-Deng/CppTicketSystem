#include <iostream>
#include "ApiServer.h"

using namespace std;

int main()
{
    cout << "========== Ticket System V5 =========="
        << endl;

    TicketSystem ticketSystem(100, 3);

    ApiServer server(ticketSystem);

    server.start(8080);

    return 0;
}
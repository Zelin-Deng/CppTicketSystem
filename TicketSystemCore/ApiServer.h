#pragma once

#include "TicketSystem.h"
#include <atomic>

class ApiServer
{
private:
    TicketSystem& ticketSystem;
    std::atomic<int> nextRequestId{ 1 };

public:
    ApiServer(TicketSystem& system);

    void start(int port);
};
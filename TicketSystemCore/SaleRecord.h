#pragma once

#include <chrono>

struct SaleRecord
{
    int ticketId;
    int userId;
    int requestId;

    std::chrono::system_clock::time_point saleTime;
};

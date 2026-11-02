#include "ApiServer.h"
#include "external/httplib.h"
#include "external/json.hpp"

#include <iostream>
#include <string>
#include <exception>

using namespace std;
using json = nlohmann::json;

namespace
{
    void sendError(httplib::Response& res,int status,const string& message)
    {

        json response;

        response["success"] = false;
        response["message"] = message;

        res.status = status;

        res.set_content(response.dump(),"application/json");
    }
}

ApiServer::ApiServer(TicketSystem& system)
    : ticketSystem(system)
{}


void ApiServer::start(int port)
{
    httplib::Server server;

    //处理全局异常
    server.set_exception_handler(
        [](const httplib::Request& req,
            httplib::Response& res,
            std::exception_ptr ep)
        {
            try
            {
                std::rethrow_exception(ep);
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Server exception: "
                    << e.what()
                    << std::endl;
            }
            catch (...)
            {
                std::cerr
                    << "Unknown server exception"
                    << std::endl;
            }

            sendError(res, 500, "Internal server error.");

        }
    );

    // Health check
    server.Get(
        "/health",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            res.set_content(
                R"({"status":"ok","message":"Ticket Server is running"})",
                "application/json"
            );
        }
    );

    // Query remaining tickets
    server.Get(
        "/tickets",
        [this](const httplib::Request& req,
            httplib::Response& res)
        {
            int remaining = ticketSystem.getRemainingTickets();

            json response;

            response["remaining_tickets"] =
                remaining;

            res.set_content(
                response.dump(),
                "application/json"
            );
        }
    );

    server.Post(
        "/tickets/purchase",

        [this](const httplib::Request& req,
            httplib::Response& res)
        {
            try
            {
                //解析 JSON
                json body = json::parse(req.body);


                //检查字段是否存在
                if (!body.contains("user_id") ||
                    !body.contains("ticket_count"))
                {
                    sendError(res, 400, "Missing required fields.");

                    return;
                }


                //检查字段类型
                if (!body["user_id"].is_number_integer() ||
                    !body["ticket_count"].is_number_integer())
                {
                    sendError(res, 400, "user_id and ticket_count must be integers.");

                    return;
                }


                //构造 TicketRequest
                TicketRequest request;

                request.requestId = nextRequestId++;
                request.userId = body["user_id"];
                request.ticketCount = body["ticket_count"];


                //提交给 TicketSystem
                future<PurchaseResult> resultFuture = ticketSystem.submitPurchaseRequest(request);

                PurchaseResult result = resultFuture.get();


                //构造 Response
                json response;

                response["success"] = result.success;
                response["message"] = result.message;
                response["ticket_ids"] = result.ticketIds;

                response["remaining_tickets"] = result.remainingTickets;

                //HTTP 状态码
                if (result.success)
                {
                    res.status = 201;
                }
                else if (result.message == "Not enough tickets")
                {
                    res.status = 409;
                }
                else
                {
                    res.status = 400;
                }


                res.set_content(
                    response.dump(),
                    "application/json"
                );
            }
            catch (const json::parse_error&)
            {
                sendError(res, 400, "Invalid JSON format.");
            }

        }
    );

    cout << "Ticket Server started on port "<< port << endl;

    server.listen("0.0.0.0", port);
}
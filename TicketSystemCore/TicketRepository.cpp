#include "TicketRepository.h"
#include "external/sqlite3.h"

#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

void TicketRepository::initDatabase()
{
    sqlite3* db = nullptr;

    try
    {
        // 1. 打开失败也要报告给 main，不能当成初始化成功返回。
        if (sqlite3_open("ticket.db", &db) != SQLITE_OK)
        {
            std::string message = db
                ? sqlite3_errmsg(db)
                : "Unable to allocate database connection";
            throw std::runtime_error("Failed to open database: " + message);
        }

        // 2. 创建库存表。
        const char* createInventorySql =
            "CREATE TABLE IF NOT EXISTS inventory ("
            "id INTEGER PRIMARY KEY,"
            "remaining_tickets INTEGER NOT NULL,"
            "next_ticket_id INTEGER NOT NULL"
            ");";

        if (sqlite3_exec(db, createInventorySql, nullptr, nullptr, nullptr)
            != SQLITE_OK)
        {
            throw std::runtime_error(
                std::string("Failed to create inventory table: ") + sqlite3_errmsg(db)
            );
        }

        // 3. 只在初始记录不存在时插入，不覆盖已有库存。
        const char* initInventorySql =
            "INSERT OR IGNORE INTO inventory "
            "(id, remaining_tickets, next_ticket_id) "
            "VALUES (1, 100, 1);";

        if (sqlite3_exec(db, initInventorySql, nullptr, nullptr, nullptr)
            != SQLITE_OK)
        {
            throw std::runtime_error(
                std::string("Failed to initialize inventory: ") + sqlite3_errmsg(db)
            );
        }

        // 4. 创建售票记录表。
        const char* createSalesSql =
            "CREATE TABLE IF NOT EXISTS sales ("
            "ticket_id INTEGER PRIMARY KEY,"
            "user_id INTEGER NOT NULL,"
            "request_id INTEGER NOT NULL,"
            "sale_time_ms INTEGER NOT NULL"
            ");";

        if (sqlite3_exec(db, createSalesSql, nullptr, nullptr, nullptr)
            != SQLITE_OK)
        {
            throw std::runtime_error(
                std::string("Failed to create sales table: ") + sqlite3_errmsg(db)
            );
        }
    }
    catch (...)
    {
        // 失败时先关闭连接，再把原异常交给 main 停止启动服务。
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        throw;
    }

    sqlite3_close(db);
    // 只有全部步骤成功后，才输出初始化完成。
    cout << "Database initialized successfully." << endl;
}

InventoryData TicketRepository::getInventory()
{
    sqlite3* db = nullptr;

    // 1. 打开数据库。
    int openResult = sqlite3_open("ticket.db", &db);

    if (openResult != SQLITE_OK)
    {
        // 关闭连接前，先把错误信息复制到 string 中。
        std::string message = db
            ? sqlite3_errmsg(db)
            : "Unable to allocate database connection";

        if (db != nullptr)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(
            "Failed to open database: " + message
        );
    }

    // 2. 准备查询语句。
    const char* sql =
        "SELECT remaining_tickets, next_ticket_id "
        "FROM inventory "
        "WHERE id = 1;";

    sqlite3_stmt* stmt = nullptr;

    int prepareResult =
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (prepareResult != SQLITE_OK)
    {
        std::string message = sqlite3_errmsg(db);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        throw std::runtime_error(
            "Failed to prepare inventory query: " + message
        );
    }

    // 3. 执行查询。
    int stepResult = sqlite3_step(stmt);

    if (stepResult != SQLITE_ROW)
    {
        // SQLITE_DONE：查询执行完了，但没有找到记录。
        // 其他结果：查询执行过程中发生了错误。
        std::string message;

        if (stepResult == SQLITE_DONE)
        {
            message = "Inventory record not found";
        }
        else
        {
            message = std::string("Failed to read inventory: ")
                + sqlite3_errmsg(db);
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        throw std::runtime_error(message);
    }

    // 4. 将查询结果复制到普通 C++ 对象中。
    InventoryData inventory{
        sqlite3_column_int(stmt, 0),
        sqlite3_column_int(stmt, 1)
    };

    // 5. 查询完成，释放 SQLite 资源。
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // 6. 把库存数据交给调用者。
    return inventory;
}

void TicketRepository::updateInventoryOnConnection(
    sqlite3* db,
    int remainingTickets,
    int nextTicketId
)
{
    sqlite3_stmt* stmt = nullptr;

    try
    {
        const char* sql =
            "UPDATE inventory "
            "SET remaining_tickets = ?, next_ticket_id = ? "
            "WHERE id = 1;";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        if (sqlite3_bind_int(stmt, 1, remainingTickets) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 2, nextTicketId) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        if (sqlite3_changes(db) != 1)
        {
            throw std::runtime_error("Inventory record not found");
        }
    }
    catch (...)
    {
        // 只释放自己的 SQL 语句，连接和事务交回 savePurchase 管理。
        sqlite3_finalize(stmt);
        throw;
    }

    sqlite3_finalize(stmt);
}

void TicketRepository::insertSaleOnConnection(
    sqlite3* db,
    const SaleRecord& record
)
{
    sqlite3_stmt* stmt = nullptr;

    try
    {
        const char* sql =
            "INSERT INTO sales "
            "(ticket_id, user_id, request_id, sale_time_ms) "
            "VALUES (?, ?, ?, ?);";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        auto saleTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            record.saleTime.time_since_epoch()
        ).count();

        if (sqlite3_bind_int(stmt, 1, record.ticketId) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 2, record.userId) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 3, record.requestId) != SQLITE_OK ||
            sqlite3_bind_int64(stmt, 4, saleTimeMs) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
    }
    catch (...)
    {
        sqlite3_finalize(stmt);
        throw;
    }

    sqlite3_finalize(stmt);
}

void TicketRepository::savePurchase(
    int remainingTickets,
    int nextTicketId,
    const std::vector<SaleRecord>& purchaseRecords
)
{
    if (remainingTickets < 0 || nextTicketId < 1 || purchaseRecords.empty())
    {
        throw std::invalid_argument("Invalid purchase state");
    }

    for (const SaleRecord& record : purchaseRecords)
    {
        if (record.ticketId < 1 || record.ticketId >= nextTicketId)
        {
            throw std::invalid_argument("Invalid sale ticket ID");
        }
    }

    sqlite3* db = nullptr;
    bool transactionStarted = false;

    try
    {
        // 1. savePurchase 统一拥有连接，辅助函数只借用同一个 db。
        if (sqlite3_open("ticket.db", &db) != SQLITE_OK)
        {
            throw std::runtime_error(
                db ? sqlite3_errmsg(db) : "Unable to open database"
            );
        }

        // 2. 开始事务后，两个辅助函数的修改都会包含在该事务中。
        if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr)
            != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        transactionStarted = true;

        // 3. 调用辅助函数更新库存，不在这里重复编写 UPDATE。
        updateInventoryOnConnection(db, remainingTickets, nextTicketId);

        // 4. 每条记录都使用同一个连接插入，辅助函数不会单独提交。
        for (const SaleRecord& record : purchaseRecords)
        {
            insertSaleOnConnection(db, record);
        }

        // 5. 所有操作成功后统一提交，提交失败也进入下面的回滚流程。
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        transactionStarted = false;
    }
    catch (...)
    {
        // 辅助函数已释放各自的语句；这里负责事务和连接的清理。
        if (db != nullptr)
        {
            // 部分 SQLite 错误会自动回滚，只撤销仍然存在的事务。
            if (transactionStarted && sqlite3_get_autocommit(db) == 0)
            {
                // 保留原始异常；关闭连接也会撤销仍未提交的事务。
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            }
            sqlite3_close(db);
        }
        throw;
    }

    sqlite3_close(db);
}

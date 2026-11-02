#pragma once

#include <vector>
#include "SaleRecord.h"

struct sqlite3;

// 数据库查询后，交给调用者的库存数据。
struct InventoryData
{
    int remainingTickets;
    int nextTicketId;
};

class TicketRepository
{
public:
    void initDatabase();

    // 查询成功：返回库存。
    // 查询失败或库存记录不存在：抛出异常。
    InventoryData getInventory();

    // 在一个事务中保存新库存和本次购票记录；purchaseRecords 仅包含本次购买。
    void savePurchase(
        int remainingTickets,
        int nextTicketId,
        const std::vector<SaleRecord>& purchaseRecords
    );

private:
    // 借用 savePurchase 的连接，仅执行 SQL 和释放自己的语句。
    // 不负责打开/关闭连接，也不提交或回滚事务。
    void updateInventoryOnConnection(
        sqlite3* db,
        int remainingTickets,
        int nextTicketId
    );

    void insertSaleOnConnection(
        sqlite3* db,
        const SaleRecord& record
    );
};
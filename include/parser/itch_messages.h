#pragma once

#include "common/types.h"
#include <cstdint>

namespace itch {

#pragma pack(push, 1)

struct SystemEventMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    char event_code;
};

struct AddOrderMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    OrderRef order_reference_number;
    char buy_sell_indicator;
    Shares shares;
    char stock[8];
    Price price;
};

struct OrderCancelMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    OrderRef order_reference_number;
    Shares cancelled_shares;
};

struct OrderExecutedMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    OrderRef order_reference_number;
    Shares executed_shares;
    uint64_t match_number;
};

struct OrderDeleteMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    OrderRef order_reference_number;
};

struct OrderReplaceMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    OrderRef original_order_reference_number;
    OrderRef new_order_reference_number;
    Shares shares;
    Price price;
};

struct TradeMessage {
    char message_type;
    StockLocate stock_locate;
    TrackingNumber tracking_number;
    uint8_t timestamp[6];
    OrderRef order_reference_number;
    char buy_sell_indicator;
    Shares shares;
    char stock[8];
    Price price;
    uint64_t match_number;
};

#pragma pack(pop)

}

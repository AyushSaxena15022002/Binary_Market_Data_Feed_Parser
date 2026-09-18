#pragma once

#include "parser/itch_messages.h"
#include <cstdint>

namespace itch {

class MessageParser {
public:
    static char get_message_type(const uint8_t* buffer);

    static void parse_add_order(const uint8_t* buffer, AddOrderMessage& msg);
    static void parse_cancel(const uint8_t* buffer, OrderCancelMessage& msg);
    static void parse_execute(const uint8_t* buffer, OrderExecutedMessage& msg);
    static void parse_delete(const uint8_t* buffer, OrderDeleteMessage& msg);
    static void parse_replace(const uint8_t* buffer, OrderReplaceMessage& msg);
    static void parse_trade(const uint8_t* buffer, TradeMessage& msg);
    static void parse_system_event(const uint8_t* buffer, SystemEventMessage& msg);

    static void print_add_order(const AddOrderMessage& msg);
};

}

#pragma once

#include "common/types.h"
#include "parser/itch_messages.h"
#include <cstring>

namespace itch {

enum class MessageType : uint8_t {
    AddOrder    = 'A',
    Cancel      = 'X',
    Execute     = 'E',
    Delete      = 'D',
    Replace     = 'U',
    Trade       = 'P',
    SystemEvent = 'S',
    Unknown     = 0
};

// Tagged union holding one fully-parsed ITCH message.
// Uses memcpy over the entire union to avoid UB with non-trivial union members.
struct ParsedMessage {
    MessageType type;

    union MsgUnion {
        AddOrderMessage    add_order;
        OrderCancelMessage cancel;
        OrderExecutedMessage execute;
        OrderDeleteMessage delete_msg;
        OrderReplaceMessage replace;
        TradeMessage       trade;
        SystemEventMessage system_event;

        MsgUnion() { std::memset(this, 0, sizeof(*this)); }
    } data;

    // Convenience accessors (aliases into union for cleaner call sites)
    AddOrderMessage&     add_order()    { return data.add_order; }
    OrderCancelMessage&  cancel()       { return data.cancel; }
    OrderExecutedMessage& execute()     { return data.execute; }
    OrderDeleteMessage&  delete_msg()   { return data.delete_msg; }
    OrderReplaceMessage& replace()      { return data.replace; }
    TradeMessage&        trade()        { return data.trade; }
    SystemEventMessage&  system_event() { return data.system_event; }

    const AddOrderMessage&     add_order()    const { return data.add_order; }
    const OrderCancelMessage&  cancel()       const { return data.cancel; }
    const OrderExecutedMessage& execute()     const { return data.execute; }
    const OrderDeleteMessage&  delete_msg()   const { return data.delete_msg; }
    const OrderReplaceMessage& replace()      const { return data.replace; }
    const TradeMessage&        trade()        const { return data.trade; }
    const SystemEventMessage&  system_event() const { return data.system_event; }

    ParsedMessage() : type(MessageType::Unknown), data() {}

    ParsedMessage(const ParsedMessage& other) : type(other.type) {
        std::memcpy(&data, &other.data, sizeof(data));
    }

    ParsedMessage& operator=(const ParsedMessage& other) {
        if (this != &other) {
            type = other.type;
            std::memcpy(&data, &other.data, sizeof(data));
        }
        return *this;
    }
};

}

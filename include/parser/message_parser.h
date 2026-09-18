#pragma once

#include "parser/itch_messages.h"
#include <cstdint>

namespace itch {

class MessageParser {
public:
    static char get_message_type(const uint8_t* buffer);
    static void parse_add_order(const uint8_t* buffer, AddOrderMessage& msg);

    static void print_add_order(const AddOrderMessage& msg);
};

}

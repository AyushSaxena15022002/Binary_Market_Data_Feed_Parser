#include "parser/message_parser.h"
#include "common/utils.h"
#include <cstring>
#include <iostream>
#include <iomanip>

namespace itch {

char MessageParser::get_message_type(const uint8_t* buffer) {
    return static_cast<char>(buffer[0]);
}

void MessageParser::parse_add_order(const uint8_t* buffer, AddOrderMessage& msg) {
    msg.message_type = static_cast<char>(buffer[0]);

    uint16_t raw16;
    uint32_t raw32;
    uint64_t raw64;

    std::memcpy(&raw16, buffer + 1, 2);
    msg.stock_locate = utils::ntoh16(raw16);

    std::memcpy(&raw16, buffer + 3, 2);
    msg.tracking_number = utils::ntoh16(raw16);

    std::memcpy(msg.timestamp, buffer + 5, 6);

    std::memcpy(&raw64, buffer + 11, 8);
    msg.order_reference_number = utils::ntoh64(raw64);

    msg.buy_sell_indicator = static_cast<char>(buffer[19]);

    std::memcpy(&raw32, buffer + 20, 4);
    msg.shares = utils::ntoh32(raw32);

    std::memcpy(msg.stock, buffer + 24, 8);

    std::memcpy(&raw32, buffer + 32, 4);
    msg.price = utils::ntoh32(raw32);
}

void MessageParser::print_add_order(const AddOrderMessage& msg) {
    uint64_t ts = utils::ntoh48(msg.timestamp);

    std::cout << "ADD ORDER | ";
    std::cout << "Ref: " << std::setw(10) << msg.order_reference_number << " | ";
    std::cout << "Side: " << msg.buy_sell_indicator << " | ";
    std::cout << "Qty: " << std::setw(6) << msg.shares << " | ";
    std::cout << "Stock: ";
    std::cout.write(msg.stock, 8);
    std::cout << " | ";
    std::cout << "Price: $" << std::fixed << std::setprecision(4)
              << (msg.price / 10000.0) << " | ";
    std::cout << "TS: " << ts << std::endl;
}

}

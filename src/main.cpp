#include "input/file_reader.h"
#include "parser/message_parser.h"
#include "parser/itch_messages.h"
#include "orderbook/book_manager.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <itch_file>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::cout << "Opening ITCH file: " << filepath << std::endl;

    itch::FileReader reader(filepath);
    if (!reader.open()) {
        return 1;
    }

    itch::BookManager book_manager;

    std::vector<uint8_t> buffer(256);
    size_t total_messages = 0;
    size_t add_orders = 0;
    size_t cancels = 0;
    size_t executes = 0;
    size_t deletes = 0;
    size_t replaces = 0;
    size_t trades = 0;
    size_t system_events = 0;
    size_t unknown_messages = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        uint16_t length = 0;
        if (!reader.read_message_length(length)) {
            break;
        }

        if (length > buffer.size()) {
            buffer.resize(length);
        }

        if (!reader.read_message_body(buffer.data(), length)) {
            break;
        }

        total_messages++;
        char msg_type = itch::MessageParser::get_message_type(buffer.data());

        switch (msg_type) {
            case 'A': {
                itch::AddOrderMessage msg;
                itch::MessageParser::parse_add_order(buffer.data(), msg);
                itch::Stock stock;
                std::memcpy(stock.symbol, msg.stock, 8);
                book_manager.on_add_order(
                    msg.order_reference_number,
                    static_cast<itch::Side>(msg.buy_sell_indicator),
                    msg.shares,
                    msg.price,
                    stock
                );
                add_orders++;
                break;
            }
            case 'X': {
                itch::OrderCancelMessage msg;
                itch::MessageParser::parse_cancel(buffer.data(), msg);
                book_manager.on_cancel_order(msg.order_reference_number, msg.cancelled_shares);
                cancels++;
                break;
            }
            case 'E': {
                itch::OrderExecutedMessage msg;
                itch::MessageParser::parse_execute(buffer.data(), msg);
                book_manager.on_execute_order(msg.order_reference_number, msg.executed_shares);
                executes++;
                break;
            }
            case 'D': {
                itch::OrderDeleteMessage msg;
                itch::MessageParser::parse_delete(buffer.data(), msg);
                book_manager.on_delete_order(msg.order_reference_number);
                deletes++;
                break;
            }
            case 'U': {
                itch::OrderReplaceMessage msg;
                itch::MessageParser::parse_replace(buffer.data(), msg);
                book_manager.on_replace_order(
                    msg.original_order_reference_number,
                    msg.new_order_reference_number,
                    msg.shares,
                    msg.price
                );
                replaces++;
                break;
            }
            case 'P': {
                itch::TradeMessage msg;
                itch::MessageParser::parse_trade(buffer.data(), msg);
                trades++;
                break;
            }
            case 'S': {
                itch::SystemEventMessage msg;
                itch::MessageParser::parse_system_event(buffer.data(), msg);
                system_events++;
                break;
            }
            default:
                unknown_messages++;
                break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n============================== PARSE STATISTICS ==============================" << std::endl;
    std::cout << "Total messages processed : " << total_messages << std::endl;
    std::cout << "  - Add Orders      ('A'): " << add_orders << std::endl;
    std::cout << "  - Cancels         ('X'): " << cancels << std::endl;
    std::cout << "  - Executions      ('E'): " << executes << std::endl;
    std::cout << "  - Deletes         ('D'): " << deletes << std::endl;
    std::cout << "  - Replaces        ('U'): " << replaces << std::endl;
    std::cout << "  - Trades          ('P'): " << trades << std::endl;
    std::cout << "  - System Events   ('S'): " << system_events << std::endl;
    if (unknown_messages > 0) {
        std::cout << "  - Unknown messages     : " << unknown_messages << std::endl;
    }
    std::cout << "Total bytes read         : " << reader.total_bytes_read() << " bytes" << std::endl;
    std::cout << "Elapsed time             : " << duration.count() << " ms" << std::endl;

    if (duration.count() > 0) {
        double msgs_per_sec = (total_messages * 1000.0) / duration.count();
        std::cout << "Throughput               : " << static_cast<size_t>(msgs_per_sec) << " msgs/sec" << std::endl;
    }

    book_manager.print_summary();

    reader.close();
    return 0;
}

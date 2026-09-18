#include "input/mmap_reader.h"
#include "input/file_reader.h"
#include "parser/message_parser.h"
#include "parser/itch_messages.h"
#include "orderbook/book_manager.h"
#include "common/utils.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <iomanip>

void run_mmap_parser(const std::string& filepath) {
    std::cout << "Opening ITCH file via zero-copy mmap: " << filepath << std::endl;

    itch::MmapReader reader(filepath);
    if (!reader.open()) {
        std::cerr << "Failed to memory-map file: " << filepath << std::endl;
        return;
    }

    itch::BookManager book_manager;

    size_t total_messages = 0;
    size_t add_orders = 0;
    size_t cancels = 0;
    size_t executes = 0;
    size_t deletes = 0;
    size_t replaces = 0;
    size_t trades = 0;
    size_t system_events = 0;

    const uint8_t* ptr = reader.data();
    const uint8_t* end = ptr + reader.size();

    auto start = std::chrono::high_resolution_clock::now();

    while (ptr + 2 <= end) {
        uint16_t length = itch::utils::ntoh16(*reinterpret_cast<const uint16_t*>(ptr));
        ptr += 2;

        if (ptr + length > end) {
            break;
        }

        total_messages++;
        char msg_type = static_cast<char>(*ptr);

        switch (msg_type) {
            case 'A': {
                itch::AddOrderMessage msg;
                itch::MessageParser::parse_add_order(ptr, msg);
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
                itch::MessageParser::parse_cancel(ptr, msg);
                book_manager.on_cancel_order(msg.order_reference_number, msg.cancelled_shares);
                cancels++;
                break;
            }
            case 'E': {
                itch::OrderExecutedMessage msg;
                itch::MessageParser::parse_execute(ptr, msg);
                book_manager.on_execute_order(msg.order_reference_number, msg.executed_shares);
                executes++;
                break;
            }
            case 'D': {
                itch::OrderDeleteMessage msg;
                itch::MessageParser::parse_delete(ptr, msg);
                book_manager.on_delete_order(msg.order_reference_number);
                deletes++;
                break;
            }
            case 'U': {
                itch::OrderReplaceMessage msg;
                itch::MessageParser::parse_replace(ptr, msg);
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
                itch::MessageParser::parse_trade(ptr, msg);
                trades++;
                break;
            }
            case 'S': {
                itch::SystemEventMessage msg;
                itch::MessageParser::parse_system_event(ptr, msg);
                system_events++;
                break;
            }
            default:
                break;
        }

        ptr += length;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start);

    std::cout << "\n======================== MMAP + ZERO-ALLOCATION PARSE ========================" << std::endl;
    std::cout << "Total messages processed : " << total_messages << std::endl;
    std::cout << "  - Add Orders      ('A'): " << add_orders << std::endl;
    std::cout << "  - Cancels         ('X'): " << cancels << std::endl;
    std::cout << "  - Executions      ('E'): " << executes << std::endl;
    std::cout << "  - Deletes         ('D'): " << deletes << std::endl;
    std::cout << "  - Replaces        ('U'): " << replaces << std::endl;
    std::cout << "  - Trades          ('P'): " << trades << std::endl;
    std::cout << "  - System Events   ('S'): " << system_events << std::endl;
    std::cout << "Total bytes mapped       : " << reader.size() << " bytes" << std::endl;
    std::cout << "Elapsed time             : " << std::fixed << std::setprecision(3)
              << (duration.count() / 1000.0) << " ms (" << duration.count() << " us)" << std::endl;

    if (duration.count() > 0) {
        double msgs_per_sec = (total_messages * 1000000.0) / duration.count();
        double avg_latency_ns = (duration.count() * 1000.0) / total_messages;
        std::cout << "Throughput               : " << static_cast<size_t>(msgs_per_sec) << " msgs/sec" << std::endl;
        std::cout << "Avg Latency per Message  : " << std::fixed << std::setprecision(1) << avg_latency_ns << " ns/msg" << std::endl;
    }

    book_manager.print_summary();
    reader.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <itch_file>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    run_mmap_parser(filepath);
    return 0;
}

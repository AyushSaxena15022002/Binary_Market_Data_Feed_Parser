#include "input/mmap_reader.h"
#include "input/file_reader.h"
#include "parser/message_parser.h"
#include "parser/itch_messages.h"
#include "parser/parsed_message.h"
#include "orderbook/book_manager.h"
#include "threading/parallel_processor.h"
#include "common/utils.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <thread>

void run_single_threaded(const std::string& filepath) {
    std::cout << "Opening ITCH file via zero-copy mmap (Single-Threaded): " << filepath << std::endl;

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

    std::cout << "\n==================== SINGLE-THREADED BASELINE ====================" << std::endl;
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

void run_multi_threaded(const std::string& filepath) {
    std::cout << "\nOpening ITCH file via zero-copy mmap (Multi-Threaded): " << filepath << std::endl;

    itch::MmapReader reader(filepath);
    if (!reader.open()) {
        std::cerr << "Failed to memory-map file: " << filepath << std::endl;
        return;
    }

    itch::BookManager book_manager;
    itch::ParallelProcessor processor(reader.data(), reader.size());

    size_t total_messages = 0;
    size_t add_orders = 0;
    size_t cancels = 0;
    size_t executes = 0;
    size_t deletes = 0;
    size_t replaces = 0;
    size_t trades = 0;
    size_t system_events = 0;

    auto start = std::chrono::high_resolution_clock::now();

    processor.start();

    // Consumer loop: process messages until producer is done AND queue is drained.
    // We use a flag-then-drain pattern to avoid the TOCTOU race where is_done()
    // becomes true between our queue empty check and the actual dequeue attempt.
    auto process_one = [&](itch::ParsedMessage& msg) {
        total_messages++;
        switch (msg.type) {
            case itch::MessageType::AddOrder: {
                itch::Stock stock;
                std::memcpy(stock.symbol, msg.data.add_order.stock, 8);
                book_manager.on_add_order(
                    msg.data.add_order.order_reference_number,
                    static_cast<itch::Side>(msg.data.add_order.buy_sell_indicator),
                    msg.data.add_order.shares,
                    msg.data.add_order.price,
                    stock
                );
                add_orders++;
                break;
            }
            case itch::MessageType::Cancel:
                book_manager.on_cancel_order(
                    msg.data.cancel.order_reference_number,
                    msg.data.cancel.cancelled_shares);
                cancels++;
                break;
            case itch::MessageType::Execute:
                book_manager.on_execute_order(
                    msg.data.execute.order_reference_number,
                    msg.data.execute.executed_shares);
                executes++;
                break;
            case itch::MessageType::Delete:
                book_manager.on_delete_order(msg.data.delete_msg.order_reference_number);
                deletes++;
                break;
            case itch::MessageType::Replace:
                book_manager.on_replace_order(
                    msg.data.replace.original_order_reference_number,
                    msg.data.replace.new_order_reference_number,
                    msg.data.replace.shares,
                    msg.data.replace.price);
                replaces++;
                break;
            case itch::MessageType::Trade:
                trades++;
                break;
            case itch::MessageType::SystemEvent:
                system_events++;
                break;
            default:
                break;
        }
    };

    itch::ParsedMessage msg;
    // Phase 1: consume while producer is still running
    while (!processor.is_done()) {
        if (processor.queue().try_dequeue(msg)) {
            process_one(msg);
        } else {
            std::this_thread::yield();
        }
    }
    // Phase 2: drain any remaining messages after producer signals done
    while (processor.queue().try_dequeue(msg)) {
        process_one(msg);
    }

    processor.stop();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start);

    std::cout << "\n==================== MULTI-THREADED (SPSC QUEUE) ====================" << std::endl;
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
        std::cerr << "Usage: " << argv[0]
                  << " <itch_file> [--multi | --compare]" << std::endl;
        std::cerr << "  (no flag)   Single-threaded baseline" << std::endl;
        std::cerr << "  --multi     Multi-threaded SPSC producer-consumer" << std::endl;
        std::cerr << "  --compare   Run both modes and print speedup table" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::string mode = (argc >= 3) ? std::string(argv[2]) : "";

    if (mode == "--multi") {
        run_multi_threaded(filepath);
    } else if (mode == "--compare") {
        // Run both and show a performance comparison table
        run_single_threaded(filepath);
        run_multi_threaded(filepath);
    } else {
        run_single_threaded(filepath);
    }

    return 0;
}

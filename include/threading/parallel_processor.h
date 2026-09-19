#pragma once

#include "common/types.h"
#include "common/spsc_queue.h"
#include "parser/parsed_message.h"
#include "parser/message_parser.h"
#include "common/utils.h"
#include <thread>
#include <atomic>
#include <memory>
#include <cstring>

namespace itch {

// Queue capacity: power-of-2, large enough to absorb bursts.
// Allocated on the heap (via unique_ptr) to avoid stack overflow.
constexpr size_t QUEUE_SIZE = 65536;

// ParallelProcessor implements a SPSC producer-consumer pattern:
//   Producer thread: reads mmap'd data, parses messages → pushes to queue.
//   Consumer thread: the calling thread dequeues and processes messages.
class ParallelProcessor {
public:
    ParallelProcessor(const uint8_t* data, size_t size)
        : data_(data), size_(size), stop_(false), producer_done_(false),
          queue_(std::make_unique<SPSCQueue<ParsedMessage, QUEUE_SIZE>>()) {}

    ~ParallelProcessor() {
        stop();
    }

    // Launch the producer thread.
    void start() {
        producer_thread_ = std::thread(&ParallelProcessor::producer_loop, this);
    }

    // Signal stop and join the producer thread.
    void stop() {
        stop_.store(true, std::memory_order_release);
        if (producer_thread_.joinable()) {
            producer_thread_.join();
        }
    }

    SPSCQueue<ParsedMessage, QUEUE_SIZE>& queue() {
        return *queue_;
    }

    // Returns true only after the producer has finished AND signalled done.
    bool is_done() const {
        return producer_done_.load(std::memory_order_acquire);
    }

private:
    void producer_loop() {
        const uint8_t* ptr = data_;
        const uint8_t* end = ptr + size_;

        while (ptr + 2 <= end && !stop_.load(std::memory_order_acquire)) {
            uint16_t length = utils::ntoh16(*reinterpret_cast<const uint16_t*>(ptr));
            ptr += 2;

            if (ptr + length > end) {
                break;
            }

            ParsedMessage msg;
            char msg_type = static_cast<char>(*ptr);

            switch (msg_type) {
                case 'A':
                    msg.type = MessageType::AddOrder;
                    MessageParser::parse_add_order(ptr, msg.data.add_order);
                    break;
                case 'X':
                    msg.type = MessageType::Cancel;
                    MessageParser::parse_cancel(ptr, msg.data.cancel);
                    break;
                case 'E':
                    msg.type = MessageType::Execute;
                    MessageParser::parse_execute(ptr, msg.data.execute);
                    break;
                case 'D':
                    msg.type = MessageType::Delete;
                    MessageParser::parse_delete(ptr, msg.data.delete_msg);
                    break;
                case 'U':
                    msg.type = MessageType::Replace;
                    MessageParser::parse_replace(ptr, msg.data.replace);
                    break;
                case 'P':
                    msg.type = MessageType::Trade;
                    MessageParser::parse_trade(ptr, msg.data.trade);
                    break;
                case 'S':
                    msg.type = MessageType::SystemEvent;
                    MessageParser::parse_system_event(ptr, msg.data.system_event);
                    break;
                default:
                    msg.type = MessageType::Unknown;
                    break;
            }

            // Spin-wait if queue is full (backpressure on consumer)
            while (!queue_->try_enqueue(msg) && !stop_.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            ptr += length;
        }

        // Signal that the producer has finished enqueuing ALL messages.
        // The consumer must drain the queue after seeing this flag.
        producer_done_.store(true, std::memory_order_release);
    }

    const uint8_t* data_;
    size_t size_;
    std::atomic<bool> stop_;
    std::atomic<bool> producer_done_;
    std::unique_ptr<SPSCQueue<ParsedMessage, QUEUE_SIZE>> queue_;
    std::thread producer_thread_;
};

}

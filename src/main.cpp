#include "input/file_reader.h"
#include "parser/message_parser.h"
#include "parser/itch_messages.h"
#include <iostream>
#include <vector>
#include <chrono>

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

    std::vector<uint8_t> buffer(256);
    size_t message_count = 0;
    size_t add_order_count = 0;

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

        message_count++;

        char msg_type = itch::MessageParser::get_message_type(buffer.data());

        if (msg_type == 'A') {
            add_order_count++;
            if (add_order_count <= 10) {
                itch::AddOrderMessage msg;
                itch::MessageParser::parse_add_order(buffer.data(), msg);
                itch::MessageParser::print_add_order(msg);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n=== Parse Summary ===" << std::endl;
    std::cout << "Total messages processed : " << message_count << std::endl;
    std::cout << "Add Order messages parsed: " << add_order_count << std::endl;
    std::cout << "Total bytes read         : " << reader.total_bytes_read() << " bytes" << std::endl;
    std::cout << "Elapsed time             : " << duration.count() << " ms" << std::endl;

    if (duration.count() > 0) {
        double msgs_per_sec = (message_count * 1000.0) / duration.count();
        std::cout << "Throughput               : " << static_cast<size_t>(msgs_per_sec) << " msgs/sec" << std::endl;
    }

    reader.close();
    return 0;
}

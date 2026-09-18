#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <cstdio>

namespace itch {

class FileReader {
public:
    explicit FileReader(const std::string& filepath);
    ~FileReader();

    bool open();
    void close();
    bool is_open() const;

    bool read_message_length(uint16_t& length);
    bool read_message_body(uint8_t* buffer, size_t length);

    size_t total_bytes_read() const { return bytes_read_; }

private:
    std::string filepath_;
    FILE* file_;
    size_t bytes_read_;
};

}

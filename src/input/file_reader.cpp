#include "input/file_reader.h"
#include "common/utils.h"
#include <iostream>

namespace itch {

FileReader::FileReader(const std::string& filepath)
    : filepath_(filepath), file_(nullptr), bytes_read_(0) {}

FileReader::~FileReader() {
    close();
}

bool FileReader::open() {
    file_ = std::fopen(filepath_.c_str(), "rb");
    if (!file_) {
        std::cerr << "Failed to open file: " << filepath_ << std::endl;
        return false;
    }
    bytes_read_ = 0;
    return true;
}

void FileReader::close() {
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }
}

bool FileReader::is_open() const {
    return file_ != nullptr;
}

bool FileReader::read_message_length(uint16_t& length) {
    uint16_t net_length;
    size_t count = std::fread(&net_length, 1, sizeof(net_length), file_);

    if (count != sizeof(net_length)) {
        return false;
    }

    length = utils::ntoh16(net_length);
    bytes_read_ += sizeof(net_length);
    return true;
}

bool FileReader::read_message_body(uint8_t* buffer, size_t length) {
    size_t count = std::fread(buffer, 1, length, file_);

    if (count != length) {
        return false;
    }

    bytes_read_ += length;
    return true;
}

}

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace itch {

class MmapReader {
public:
    explicit MmapReader(const std::string& filepath);
    ~MmapReader();

    bool open();
    void close();
    bool is_open() const { return data_ != nullptr; }

    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }

private:
    std::string filepath_;
    const uint8_t* data_;
    size_t size_;

#ifdef _WIN32
    HANDLE file_handle_;
    HANDLE mapping_handle_;
#else
    int fd_;
#endif
};

}

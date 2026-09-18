#include "input/mmap_reader.h"
#include <iostream>

namespace itch {

MmapReader::MmapReader(const std::string& filepath)
    : filepath_(filepath), data_(nullptr), size_(0)
#ifdef _WIN32
    , file_handle_(INVALID_HANDLE_VALUE), mapping_handle_(NULL)
#else
    , fd_(-1)
#endif
{}

MmapReader::~MmapReader() {
    close();
}

bool MmapReader::open() {
#ifdef _WIN32
    file_handle_ = CreateFileA(
        filepath_.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );

    if (file_handle_ == INVALID_HANDLE_VALUE) {
        std::cerr << "MmapReader: Failed to open file: " << filepath_ << std::endl;
        return false;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }
    size_ = static_cast<size_t>(file_size.QuadPart);

    if (size_ == 0) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    mapping_handle_ = CreateFileMappingA(
        file_handle_,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (mapping_handle_ == NULL) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    data_ = static_cast<const uint8_t*>(MapViewOfFile(
        mapping_handle_,
        FILE_MAP_READ,
        0,
        0,
        size_
    ));

    if (data_ == nullptr) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = NULL;
        file_handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
#else
    fd_ = ::open(filepath_.c_str(), O_RDONLY);
    if (fd_ < 0) {
        std::cerr << "MmapReader: Failed to open file: " << filepath_ << std::endl;
        return false;
    }

    struct stat sb;
    if (fstat(fd_, &sb) < 0 || sb.st_size == 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    size_ = static_cast<size_t>(sb.st_size);

    void* addr = ::mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (addr == MAP_FAILED) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    madvise(addr, size_, MADV_SEQUENTIAL | MADV_WILLNEED);
    data_ = static_cast<const uint8_t*>(addr);
    return true;
#endif
}

void MmapReader::close() {
#ifdef _WIN32
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (mapping_handle_) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = NULL;
    }
    if (file_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (data_) {
        munmap(const_cast<uint8_t*>(data_), size_);
        data_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    size_ = 0;
}

}

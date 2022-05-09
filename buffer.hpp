#pragma once

#include <stdint.h>
#include <stddef.h>
#include <utility>

class buffer
{
    uint8_t * ptr = nullptr;
    size_t size = 0;

public:
    buffer(size_t bytes)
    {
        ptr = new uint8_t[bytes];
        size = bytes;
    }

    buffer(buffer&& other)
    {
        size = std::exchange(other.size, size);
        ptr = std::exchange(other.ptr, ptr);

        if(other.ptr) {
            delete [] other.ptr;
            other.ptr = nullptr;
            other.size = 0;
        }
    }

    // ni ma!
    buffer(buffer& other) = delete;
    buffer(const buffer& other) = delete;

    uint8_t* data() const {
        return ptr;
    }

    auto getSize() const {
        return size;
    }

    ~buffer()
    {
        if(ptr)
            delete [] ptr;
    }
};
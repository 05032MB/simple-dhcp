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

    void trimToSize(size_t size) {
        if(size >= this->size)
            return;
        
        this->size = size;
        /*uint8_t * newptr = new uint8_t[size];
        memcpy(newptr, ptr, size);
        std::swap(ptr, newptr);
        delete [] newptr;*/
    }

    buffer concat(const buffer &other) {
        buffer ret(other.getSize() + getSize());
        memcpy(ret.ptr, ptr, size);
        memcpy(ret.ptr, other.ptr + size, other.size);
        return ret;
    }

    uint8_t* data() const {
        return ptr;
    }

    int getSize() const {
        return size;
    }

    ~buffer()
    {
        if(ptr)
            delete [] ptr;
    }

    friend class udpsocketserver;
};
#pragma once

#include <string.h>
#include <stdexcept>
#include <string>

#define THROW_RUNTIME_GET_ERRNO(what) \
    throw std::runtime_error( ( what ": ") + std::string(strerror(errno)) )

struct addrCmp {
    bool operator() (const in_addr& a, const in_addr& b) const { 
        return a.s_addr < b.s_addr; 
    }
};
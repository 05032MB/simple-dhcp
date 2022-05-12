#pragma once

#include "misc.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <stdexcept>
#include <set>

class addrpool {
    // they are in network endianess
    in_addr low, high;
    in_addr current;
    int maxsize;

    std::set<in_addr, addrCmp> usedAddrs;

    void commonInit() {
        if(ntohl(this->low.s_addr) > ntohl(this->high.s_addr))
            std::swap(this->low, this->high);

        maxsize = ntohl(this->high.s_addr) - ntohl(this->low.s_addr) + 1;
        current.s_addr = htonl(ntohl(this->low.s_addr) - 1);
    }

public:
    addrpool(in_addr_t low, in_addr_t high) {
        this->low = {low};
        this->high = {high};

        commonInit();
    }

    addrpool(std::string low, std::string high) {
        if(inet_aton(low.c_str(), &this->low) <= 0) {
            THROW_RUNTIME_GET_ERRNO("inet_aton failed: ");
        }

        if(inet_aton(high.c_str(), &this->high) <= 0) {
            THROW_RUNTIME_GET_ERRNO("inet_aton failed: ");
        }

        commonInit();
    }

    in_addr getFreeAddr() {
        if(usedAddrs.size() == this->maxsize)
            throw std::runtime_error("Address pool has no more free addresses.");

        auto incrCurrent = [&]() {current.s_addr = htonl(ntohl(current.s_addr) + 1);};
        do {
            incrCurrent();

            if(ntohl(current.s_addr) > ntohl(high.s_addr)) {
                current = low;
            }
            
        } while(usedAddrs.count(current) != 0);
        
        usedAddrs.insert(current);
        return current;
    }

    bool freeAddr(const in_addr& addr) {
        return usedAddrs.erase(addr) == 1;
    }

    bool isFreeAddr(const in_addr& addr) const {
        return usedAddrs.count(addr) == 0;
    }

    void claimAddr(const in_addr& addr) {
        usedAddrs.insert(addr);
    }

    int getFreeAddrsCount() const {
        return maxsize - usedAddrs.size();
    }

};
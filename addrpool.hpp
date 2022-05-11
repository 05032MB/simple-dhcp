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

    struct addrCmp {
        bool operator() (const in_addr& a, const in_addr& b) const { 
            return a.s_addr < b.s_addr; 
        }
    };
    std::set<in_addr, addrCmp> usedAddrs;

    void commonInit() {
        if(ntohl(this->low.s_addr) > ntohl(this->high.s_addr))
            std::swap(this->low, this->high);

        maxsize = ntohl(this->high.s_addr) - ntohl(this->low.s_addr) + 1;
        current.s_addr = htonl(ntohl(this->low.s_addr) - 1);

        //std::cout << ntohl(low.s_addr) << " " << ntohl(high.s_addr) << std::endl;
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
            //std::cout << ntohl(current.s_addr) << "/" << ntohl(high.s_addr) << std::endl;
            if(ntohl(current.s_addr) > ntohl(high.s_addr)) {
                current = low;
            }
            
        } while(usedAddrs.count(current) != 0);
        
        usedAddrs.insert(current);
        return current;
    }

    void freeAddr(in_addr addr) {
        std::cout << usedAddrs.erase(addr);
    }

};
#pragma once

#include <stdint.h>
#include <cstring>
#include <vector>

#include "buffer.hpp"
#include "dhcpopts.hpp"

#define __packed __attribute__((packed))


// http://www.tcpipguide.com/free/t_DHCPMessageFormat.htm
class dhcpmsg
{
public:

    dhcpmsg() = default;

    buffer getMemoryBlock() {
        size_t optionsLen = 0;
        for(const auto &i : options)
        {
            optionsLen += i.getSize();
        }
        auto buff = buffer(sizeof(header) + optionsLen);

        std::memcpy(buff.data(), &header, sizeof(header));
        auto *rawptr = buff.data() + sizeof(header);

        size_t off = 0;
        for(const auto& i : options) {
            rawptr[off++] = i.getCode();
            rawptr[off++] = i.getParamsSize();

            for(auto it = begin(i.params); it != end(i.params); ++it) {
                std::memcpy(rawptr + off, &(*it), sizeof(decltype(*it)) );
                off += sizeof(decltype(*it));
            }
        }

        return buff;
    }

    template<class T>
    static dhcpmsg makeDhcpMsg(const T* mem) {
        const uint8_t *memory = reinterpret_cast<const uint8_t *>(mem);

        dhcpmsg rets;
        memcpy(&rets.header, mem, sizeof(rets.header));
        return rets;
    }

    enum class boot_t : uint8_t
    {
        BOOTREQUEST = 1,
        BOOTREPLY = 2
    };

    struct __packed msghdr {
        boot_t op;
        // https://community.plus.net/t5/Tech-Help-Software-Hardware-etc/DHCP-Hardware-types/td-p/1494684
        uint8_t hardware_type = 1; // Ethernet
        uint8_t hardware_addr_len = 6; // MAC Len
        uint8_t hops = 0;

        uint32_t tx_id = 0;

        uint16_t secs = 0, flags = 0;

        uint32_t client_ip_addr = 0, your_ip_addr = 0, server_ip_addr = 0, gateway_ip_addr = 0;
        
        uint8_t client_hw_addr[6];
        uint8_t client_hw_addr_zero_1[2] = {0};

        uint32_t client_hw_addr_zero_2_3[2] = {0};
        uint8_t zeros[192] = {0};

        uint32_t cookie = 0x63825363;
    };

    const *msghdr getHeader() const {

    }

private:

    template <class T>
    inline void assignAndAdvance(T& target, const T* src) {
        target = *src;
        src += sizeof(T);
    }

    msghdr header;
    std::vector<dhcpopt> options = {};

};
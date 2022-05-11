#pragma once

#include "addrpool.hpp"
#include "dhcpmsg.hpp"
#include "misc.hpp"
#include "socket.hpp"
#include "dhcpopts.hpp"

#include <unistd.h>
#include <string>
#include <tuple>
#include <iostream>

class dhcpsrvc{
    std::string iface;

    addrpool ipv4Pool;
    udpsocketserver socketListener;

    static void dumppkt(const buffer &h) {
        int len = h.getSize();
        printf("[%dB]\n", len);
        const auto* bytes = h.data();
        for (int i = 0; i < len; i += 16) {
            printf("0x%04x:  ", i);
            for (int j = 0; j < 16; j++) {
                unsigned char ch = (i + j < len) ? bytes[i + j] : 0;
                if (i + j < len) printf("%02x ", ch);
                else printf("   ");
            }
            printf(" ");
            for (int j = 0; j < 16; j++) {
                unsigned char ch = (i + j < len) ? bytes[i + j] : ' ';
                if (!isprint(ch)) ch = '.';
                    printf("%c", ch);
            }
            printf("\n");
        }
    }

public:
    dhcpsrvc(std::string iface, std::string lowIp, std::string highIp) 
        : ipv4Pool(lowIp, highIp) {
        this->iface = iface;
        socketListener.bind(67);
    }

    void sendTestNak() {
        dhcpmsg testmsg;
        testmsg.getHeader().tx_id = htonl(0x2137);
        testmsg.getHeader().op = dhcpmsg::boot_t::BOOTREPLY;

        testmsg.addOption(DHCPNAK);
        testmsg.addOption(dhcpopt::makeDhcpOpt<51, 0xff, 0xff, 0xff, 0xff>());
        testmsg.addOption(OPTS_END);

        std::cout << socketListener.send(testmsg.getMemoryBlock(), {AF_INET, htons(68), htonl(3232236040), 0});
        std::cout << "sent" << std::endl;

    }

    void run() {
        sendTestNak();

        auto data = socketListener.receive();
        int size = std::get<2>(data);
        while (size != 0) {
            auto& bytes = std::get<0>(data);
            bytes.trimToSize(size);
            auto msg = dhcpmsg::makeDhcpMsg(bytes.data());

            //std::cout << std::hex << +msg.getHeader().client_hw_addr[0] << std::endl;

            //dumppkt(bytes);

            /*auto* pos = bytes.data() + sizeof(dhcpmsg::msghdr);
            while(pos < pos + size) {
                auto testopts = dhcpopt::makeDhcpOpt(pos);
                if(testopts.getCode() == 255) {
                    std::cout << "END" << std::endl;
                    break;
                }
                //std::cout << "??" << sizeof(dhcpmsg::msghdr) << std::endl;

                //printf("%02x ", testopts.getCode());

                std::cout << "Code: " << std::hex << +testopts.getCode() << " size: " << +testopts.getParamsSize() << "\n";
                for(const auto &i : testopts.getParams()) {
                    std::cout << std::hex << +i << std::endl;
                }
                
                pos += testopts.getSize();
            }*/

            auto data = socketListener.receive();
            size = std::get<2>(data);
        }
    }
};
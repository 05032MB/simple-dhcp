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
#include <algorithm>
#include <memory>
#include <map>

// https://www.netmanias.com/en/post/techdocs/5998/dhcp-network-protocol/understanding-the-basic-operations-of-dhcp
// https://www.netmanias.com/en/?m=view&id=techdocs&no=5999
class dhcpsrvc{
    std::string iface;

    addrpool ipv4Pool;
    udpsocketserver socketListener;

    enum class offerStatus {
        offered,
        assigned
    };

    std::map<in_addr, offerStatus, addrCmp> leasedAddrs;

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

    void dhcpDiscoverHandler(const dhcpmsg &in, const sockaddr_in &src) {
        dhcpmsg reply;
        std::memcpy(reply.getHeader().client_hw_addr, in.getHeader().client_hw_addr, 6);
        reply.getHeader().tx_id = in.getHeader().tx_id;
        reply.addOption(DHCPOFFER);
        
        auto candidateAddr = ipv4Pool.getFreeAddr();
        leasedAddrs.insert({candidateAddr, offerStatus::offered});
        reply.getHeader().client_ip_addr = candidateAddr.s_addr;

        reply.signOff();

        sockaddr_in target = {AF_INET, htons(68), INADDR_BROADCAST, 0};
        if(socketListener.send(reply.getMemoryBlock(), target) <= 0) {
            std::cout << "Couldn't reply with DHCPOFFER" << std::string(strerror(errno)) << std::endl;
        }
    }

    void dhcpRequestHandler(const dhcpmsg &in, const sockaddr_in &src) {
        dhcpmsg reply;
        std::memcpy(reply.getHeader().client_hw_addr, in.getHeader().client_hw_addr, 6);
        reply.getHeader().tx_id = in.getHeader().tx_id;
        
        auto reqAddr = std::find_if(std::begin(in.getOptions()), std::end(in.getOptions()), [](const dhcpopt& o) { return o.getCode() == 50; });
        if(reqAddr == std::end(in.getOptions())) {
            reply.addOption(DHCPNAK); // no ip requested
        } else {
            
            in_addr addr = {0};
            for(const auto &i : reqAddr->getParams()) {
                addr.s_addr <<= 8;
                addr.s_addr |= i;
            }
            addr.s_addr = htonl(addr.s_addr);
            reply.getHeader().client_ip_addr = addr.s_addr;

            if(ipv4Pool.isFreeAddr(addr)) {
                ipv4Pool.claimAddr(addr);
                leasedAddrs[addr] = offerStatus::offered;
                reply.addOption(DHCPACK);
            } else {
                auto leased = leasedAddrs.find(addr);
                if(leased != std::end(leasedAddrs) && leased->second == offerStatus::offered) {
                    leased->second = offerStatus::assigned;
                    reply.addOption(DHCPACK);
                } else {
                    reply.addOption(DHCPNAK);
                }
            }

            /*auto candidateAddr = ipv4Pool.getFreeAddr();
            leasedAddrs.insert({candidateAddr, offerStatus::offered});
            reply.getHeader().client_ip_addr = candidateAddr.s_addr;*/
        }

        auto gatewayIp = OPT_DEFAULT_GATEWAY_IP;
        auto dnsIp = OPT_DEFAULT_DNS;
        auto gatewayAddr = inet_addr("192.168.1.1"); // hack, fix it!
        memcpy(&gatewayIp.params[0], &gatewayAddr, 4);
        memcpy(&dnsIp.params[0], &gatewayAddr, 4);
        reply.addOption(gatewayIp);
        reply.addOption(dnsIp);
        reply.signOff();

        sockaddr_in target = {AF_INET, htons(68), INADDR_BROADCAST, 0};
        if(socketListener.send(reply.getMemoryBlock(), target) <= 0) {
            std::cout << "Couldn't reply with DHCPOFFER" << std::string(strerror(errno)) << std::endl;
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

        //auto data = socketListener.receive();
        //int size = std::get<2>(data);

        using rettype = std::pair<buffer&, sockaddr_in&>;
        for (auto data = std::make_unique<rettype>(socketListener.receive()); 
                std::get<0>(*data).getSize() != 0; data = std::make_unique<rettype>(socketListener.receive()) ) {
            
            auto& bytes = std::get<0>(*data);
            if(bytes.getSize() < sizeof(dhcpmsg::msghdr)) {
                std::cout << "Skipped packet too short to be DHCP." << std::endl;
                continue;
            }

            auto msg = dhcpmsg::makeDhcpMsg(bytes);

            /*
            for(const auto &i : msg.getOptions()) {
                std::cout << std::hex << +i.getCode() << "s " << +i.getParamsSize() << std::dec << std::endl;
            }
            dumppkt(bytes);
            */

            const auto& opts = msg.getOptions();
            const auto dhcpcommand = std::find_if(std::begin(opts), std::end(opts), [] (const dhcpopt &o) { return o.getCode() == 53; });

            //std::cout << "psize" << +dhcpcommand->getParamsSize() << std::endl;
            if(dhcpcommand->getParamsSize() > 1) {
                std::cout << "Skipped malformed DHCP packet " << std::endl;
                continue;
            }

            switch(dhcpcommand->getParams().back()) {
                case 1:
                    std::cout << "Handling DHCPDISCOVER" << std::endl;
                    dhcpDiscoverHandler(msg, data->second);
                    break;
                case 3:
                    std::cout << "Handling DHCPREQUEST" << std::endl;
                    dhcpRequestHandler(msg, data->second);
                    break;
            }
        }
    }
};
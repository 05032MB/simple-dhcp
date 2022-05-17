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
    in_addr gateway, subnet;
    std::vector<in_addr> dns;

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

    static in_addr getAddrFromParams(const dhcpopt &reqAddr) {
        in_addr addr = {0};
        for(const auto &i : reqAddr.getParams()) {
            addr.s_addr <<= 8;
            addr.s_addr |= i;
        }
        addr.s_addr = htonl(addr.s_addr);

        return addr;
    }

    static auto findFirstOptOfType(const dhcpmsg &in, uint8_t optCode) {
        return std::find_if(std::begin(in.getOptions()), std::end(in.getOptions()), [&optCode](const dhcpopt& o) { return o.getCode() == optCode; });
    }

    void dhcpDiscoverHandler(const dhcpmsg &in, const sockaddr_in &src [[maybe_unused]]) {
        dhcpmsg reply;
        std::memcpy(reply.getHeader().client_hw_addr, in.getHeader().client_hw_addr, 6);
        reply.getHeader().tx_id = in.getHeader().tx_id;
        
        if(ipv4Pool.getFreeAddrsCount() == 0) {
            reply.addOption(DHCPNAK);
        } else {
            reply.addOption(DHCPOFFER);
            auto candidateAddr = ipv4Pool.getFreeAddr();
            leasedAddrs.insert({candidateAddr, offerStatus::offered});
            reply.getHeader().client_ip_addr = candidateAddr.s_addr;
        }

        auto subnetOpt = OPT_SUBNET_MASK;
        memcpy(&subnetOpt.params[0], &this->subnet, 4);
        reply.addOption(subnetOpt);

        reply.signOff();

        sockaddr_in target = {AF_INET, htons(68), INADDR_BROADCAST, 0};
        if(socketListener.send(reply.getMemoryBlock(), target) <= 0) {
            std::cout << "Couldn't reply with DHCPOFFER: " << std::string(strerror(errno)) << std::endl;
        }
    }

    void dhcpRequestHandler(const dhcpmsg &in, const sockaddr_in &src [[maybe_unused]]) {
        dhcpmsg reply;
        std::memcpy(reply.getHeader().client_hw_addr, in.getHeader().client_hw_addr, 6);
        reply.getHeader().tx_id = in.getHeader().tx_id;
        
        auto reqAddr = findFirstOptOfType(in, 50);
        if(reqAddr == std::end(in.getOptions())) {
            reply.addOption(DHCPNAK); // no ip requested
        } else {
            
            in_addr addr = getAddrFromParams(*reqAddr);
            reply.getHeader().client_ip_addr = addr.s_addr;

            if(ipv4Pool.isFreeAddr(addr)) {
                ipv4Pool.claimAddr(addr);
                leasedAddrs[addr] = offerStatus::assigned;
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
        }

        auto subnetOpt = OPT_SUBNET_MASK;
        memcpy(&subnetOpt.params[0], &this->subnet, 4);
        reply.addOption(subnetOpt);

        for(const auto& dnsAddr : this->dns) {
            auto dnsIp = OPT_DEFAULT_DNS;
            memcpy(&dnsIp.params[0], &dnsAddr, 4);
            reply.addOption(dnsIp);
        }

        auto gatewayIp = OPT_DEFAULT_GATEWAY_IP;
        memcpy(&gatewayIp.params[0], &this->gateway, 4);
        reply.addOption(gatewayIp);
        reply.signOff();

        sockaddr_in target = {AF_INET, htons(68), INADDR_BROADCAST, 0};
        if(socketListener.send(reply.getMemoryBlock(), target) <= 0) {
            std::cout << "Couldn't reply with DHCPACK: " << std::string(strerror(errno)) << std::endl;
        }
    }

    void dhcpReleaseHandler(const dhcpmsg &in, const sockaddr_in &src [[maybe_unused]]) {
        in_addr addr = {0};
        memcpy(&addr, &in.getHeader().client_ip_addr, 4);

        ipv4Pool.freeAddr(addr);
        leasedAddrs.erase(addr);
    }

public:
    dhcpsrvc(const std::string& iface, const std::string& lowIp, const std::string& highIp, 
        const std::string& gateway, const std::string& subnetMask, const std::vector<std::string>& dns = {}) 
        : ipv4Pool(lowIp, highIp) {

        this->iface = iface;

        if(inet_aton(gateway.c_str(), &this->gateway) <= 0) {
            THROW_RUNTIME_GET_ERRNO("inet_aton failed: ");
        }

        if(inet_aton(subnetMask.c_str(), &this->subnet) <= 0) {
            THROW_RUNTIME_GET_ERRNO("inet_aton failed: ");
        }

        for(const auto &i: dns) {
            in_addr tmp;
            if(inet_aton(i.c_str(), &tmp) <= 0) {
                THROW_RUNTIME_GET_ERRNO("inet_aton failed: ");
            }
            this->dns.push_back(tmp);
        }

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
        //sendTestNak();

        using rettype = std::pair<const buffer&, const sockaddr_in&>;
        for (auto data = std::make_unique<rettype>(socketListener.receive()); 
                std::get<0>(*data).getSize() != 0; data = std::make_unique<rettype>(socketListener.receive()) ) {
            
            auto& bytes = std::get<0>(*data);
            if(bytes.getSize() < sizeof(dhcpmsg::msghdr)) {
                std::cout << "Skipped packet too short to be DHCP." << std::endl;
                continue;
            }

            auto msg = dhcpmsg::makeDhcpMsg(bytes);
            const auto dhcpcommand = findFirstOptOfType(msg, DHCP_MSG);

            if(dhcpcommand == std::end(msg.getOptions()) || dhcpcommand->getParamsSize() > 1) {
                std::cout << "Skipped malformed DHCP packet " << std::endl;
                continue;
            }

            switch(dhcpcommand->getParams().back()) {
                case DHCPCODES::DISCOVER :
                    std::cout << "Handling DHCPDISCOVER" << std::endl;
                    dhcpDiscoverHandler(msg, data->second);
                    break;
                case DHCPCODES::REQUEST:
                    std::cout << "Handling DHCPREQUEST" << std::endl;
                    dhcpRequestHandler(msg, data->second);
                    break;
                case DHCPCODES::RELEASE:
                    std::cout << "Handling DHCPRELEASE" << std::endl;
                    dhcpReleaseHandler(msg, data->second);
                    break;
                default:
                    std::cout << "Idk" << std::endl;
                    break;
            }
        }
    }
};
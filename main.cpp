#include "dhcpmsg.hpp"
#include "dhcpopts.hpp"
#include "socket.hpp"
#include "addrpool.hpp"
#include "dhcpsrvc.hpp"

#include <iostream>
#include <sstream>

std::string getEnvOrEmpty(const std::string &name) {
    // https://stackoverflow.com/questions/4237812/should-i-free-delete-char-returned-by-getenv - tl;dr - no
    char* str = getenv(name.c_str());
    return str != nullptr ? std::string(str) : "";
}

int main(int argc, char *argv[])
{
    std::string addr = "";
    if(argc >= 2) {
        addr = argv[1];
    }
    
    try {
        auto servs = getEnvOrEmpty("DHCP_APP_DNS_SERVERS");
        std::stringstream splitter(servs);
        std::vector<std::string> dnsAddrs;

        std::string tmps;
        while(std::getline(splitter, tmps, ';')) {
            dnsAddrs.push_back(tmps);
        }

        dhcpsrvc service(addr, getEnvOrEmpty("DHCP_APP_POOL_LOW"), getEnvOrEmpty("DHCP_APP_POOL_HIGH"), 
            getEnvOrEmpty("DHCP_APP_GATEWAY"), getEnvOrEmpty("DHCP_APP_SUBNET_MASK"), dnsAddrs);
        
        std::cout << "Service starting..." << std::endl;
        service.run();
    } catch(const std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
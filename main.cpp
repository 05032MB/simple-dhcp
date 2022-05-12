#include "dhcpmsg.hpp"
#include "dhcpopts.hpp"
#include "pcapwrapper.hpp"
#include "socket.hpp"
#include "addrpool.hpp"
#include "dhcpsrvc.hpp"

#include <iostream>
#include <sstream>

std::string getEnvOrEmpty(const std::string &name) {
    char* str = getenv(name.c_str());
    return str != nullptr ? std::string(str) : "";
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " interface " << std::endl;
        return -1;
    }
    
    try {
        auto servs = getEnvOrEmpty("DHCP_APP_DNS_SERVERS");
        std::stringstream splitter(servs);
        std::vector<std::string> dnsAddrs;

        std::string tmps;
        while(std::getline(splitter, tmps, ';')) {
            dnsAddrs.push_back(tmps);
        }

        dhcpsrvc service(argv[1], getEnvOrEmpty("DHCP_APP_POOL_LOW"), getEnvOrEmpty("DHCP_APP_POOL_HIGH"), 
            getEnvOrEmpty("DHCP_APP_GATEWAY"), dnsAddrs);
        
        service.run();
    } catch(const std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
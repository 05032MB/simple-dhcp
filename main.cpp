#include "dhcpmsg.hpp"
#include "dhcpopts.hpp"
#include "pcapwrapper.hpp"
#include "socket.hpp"
#include "addrpool.hpp"
#include "dhcpsrvc.hpp"

#include <iostream>

int main(int argc, char *argv[])
{
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " interface " << std::endl;
        return -1;
    }
    
    try {
        dhcpsrvc service(argv[1], "192.168.1.101", "192.168.1.191");
        service.run();
    } catch(const std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
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
        /*pcap_wrapper dhcpListener(argv[1]);

        dhcpListener.compile("udp and src port 68 and dst port 67");

        auto rawHandle = dhcpListener.getHandle();

        struct pcap_pkthdr *h;
        const u_char *bytes;
        int res = pcap_next_ex(rawHandle, &h, &bytes);

        printf("[%dB of %dB]\n", h->caplen, h->len);

        int len = h->len;
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

        */

       /*
        addrpool poolIp4("192.168.2.100", "192.168.2.191");

        int ads = 11;
        while(--ads) {
            auto caddr = poolIp4.getFreeAddr();
            std::cout << ntohl(caddr.s_addr) << " " << inet_ntoa(caddr) << std::endl;
        }
        poolIp4.freeAddr({htonl(3232236040)});
        std::cout << "freed" << std::endl;
        auto caddr = poolIp4.getFreeAddr();
        std::cout << ntohl(caddr.s_addr) << " " << inet_ntoa(caddr) << std::endl;
        */

        //udpsocketserver serverPrimitive;
        //serverPrimitive.bind(67);

        dhcpsrvc service(argv[1], "192.168.2.100", "192.168.2.191");
        service.run();


    } catch(const std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
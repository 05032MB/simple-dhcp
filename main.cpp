#include <iostream>
#include "dhcpmsg.hpp"
#include "dhcpopts.hpp"
#include "pcapwrapper.hpp"
#include "socket.hpp"

void dumppkt(const buffer &h) {
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

int main(int argc, char *argv[])
{
    if(argc < 2)
        return -1;
    
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

        udpsocketserver serverPrimitive;
        serverPrimitive.bind(67);

        auto data = serverPrimitive.receive();
        int size = std::get<2>(data);
        while (size != 0) {
            auto& bytes = std::get<0>(data);
            bytes.trimToSize(size);
            auto msg = dhcpmsg::makeDhcpMsg(bytes.data());

            dumppkt(bytes);

            auto* pos = bytes.data() + sizeof(dhcpmsg::msghdr);
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
            }

            auto data = serverPrimitive.receive();
            size = std::get<2>(data);
        }


    } catch(const std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
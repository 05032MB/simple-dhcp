#include <pcap/pcap.h>

class pcap_wrapper
{
    pcap_t *handle = nullptr;
    std::string errbuff{PCAP_ERRBUF_SIZE + 1, 0};

public:

    pcap_wrapper(std::string iface, )
    {
        handle = pcap_create(iface, &errbuff[0]);
    }

    ~pcap_wrapper()
    {
        if(handle)
            delete [] handle;
    }
};
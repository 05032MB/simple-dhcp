#include <pcap/pcap.h>

#include <string>
#include <exception>

class pcap_wrapper
{
    bpf_u_int32 netp, maskp;
    bpf_program fp{0, nullptr};

    pcap_t *handle = nullptr;
    std::string errbuff{PCAP_ERRBUF_SIZE + 1, '\0', std::string::allocator_type{}};

public:

    struct pcap_except : std::exception
    {
        std::string msg;

        pcap_except(std::string fun, std::string more) {
            msg = fun + ": " + more;
        }

        const char* what() const noexcept override {
            return msg.c_str();
        }
    };

    pcap_wrapper(std::string iface)
    {
        handle = pcap_create(iface.c_str(), &errbuff[0]);
        if(handle == nullptr)
            throw pcap_except{"create", errbuff};
        //pcap_set_promisc(handle, 1);
        pcap_set_snaplen(handle, 65535);
        pcap_set_timeout(handle, 1000);

        if(pcap_activate(handle) < 0)
            throw pcap_except{"activate", pcap_geterr(handle)};
        
        if(pcap_lookupnet(iface.c_str(), &netp, &maskp, &errbuff[0]) < 0)
            throw pcap_except{"lookupnet", errbuff};
    
    }

    void compile(std::string program, bool nonblock = false) {
        if(pcap_compile(handle, &fp, program.c_str(), 0, maskp) < 0)
            throw pcap_except{"compile", pcap_geterr(handle)};

        if (pcap_setfilter(handle, &fp) < 0)
            throw pcap_except{"setfilter", pcap_geterr(handle)};

        if(nonblock) {
            if(pcap_setnonblock(handle, 1, &errbuff[0]))
                throw pcap_except{"nonblock", errbuff};
        }

    }

    pcap_t * getHandle() const {
        return handle;
    }

    ~pcap_wrapper()
    {
        if(fp.bf_insns != nullptr)
            pcap_freecode(&fp);
        if(handle)
            pcap_close(handle);
    }
};
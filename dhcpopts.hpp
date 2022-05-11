#pragma once

#include <stdint.h>
#include <vector>
#include <array>
#include <memory>

class dhcpopt
{
    uint8_t code = 0;

    public:
        dhcpopt(uint8_t code) {
            this->code = code;
        }

        template<uint8_t code, uint8_t ...opts>
        static dhcpopt makeDhcpOpt() {
            dhcpopt opt(code);
            opt.params = {opts...};
            return opt;
        }

        template<class T>
        static dhcpopt makeDhcpOpt(const T* mem) {
            const uint8_t *memory = reinterpret_cast<const uint8_t *>(mem);
            uint8_t code = *memory;
            uint8_t size = *(memory + sizeof(code));

            dhcpopt opt(code);
            if(code == 255) { // END has no length field
                return opt;
            }
            
            for(const uint8_t * i = memory + sizeof(code) + sizeof(size); 
                i < reinterpret_cast<const uint8_t *>(memory + sizeof(code) + sizeof(size) + size); i++) {
                    opt.params.push_back(*i);
            }

            return opt;
        }

        buffer getBuffer() const {
            buffer ret(getSize());

            auto * rawptr = ret.data();
            rawptr[0] = code;
            rawptr[1] = getParamsSize();
            
            for(size_t i = 0; i < params.size(); i++) {
                rawptr[2 + i] = params[i];
            }

            return ret;
        }

        uint8_t getCode() const {
            return code;
        }

        const auto& getParams() const {
            return params;
        }

        uint8_t getParamsSize() const {
            return params.size();
        }

        uint8_t getSize() const {
            return sizeof(code) + sizeof(decltype(getParamsSize())) + getParamsSize();
        }

        std::vector<uint8_t> params = {};

        friend class dhcpmsg;
};


// common options aliases

#define DHCPOPTS_GEN_TYPE(code, ...) \
    dhcpopt::makeDhcpOpt<code, __VA_ARGS__>()

#define DHCPOPTS_GEN_DHCP_TYPE(...) \
    DHCPOPTS_GEN_TYPE(53, __VA_ARGS__)

// https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol#DHCP_message_types
#define	DHCPDISCOVER          DHCPOPTS_GEN_DHCP_TYPE(1)
#define	DHCPOFFER             DHCPOPTS_GEN_DHCP_TYPE(2)
#define	DHCPREQUEST           DHCPOPTS_GEN_DHCP_TYPE(3)
#define	DHCPDECLINE           DHCPOPTS_GEN_DHCP_TYPE(4)
#define	DHCPACK               DHCPOPTS_GEN_DHCP_TYPE(5)
#define	DHCPNAK               DHCPOPTS_GEN_DHCP_TYPE(6)
#define	DHCPRELEASE           DHCPOPTS_GEN_DHCP_TYPE(7)
#define	DHCPINFORM            DHCPOPTS_GEN_DHCP_TYPE(8)
#define	DHCPFORCERENEW        DHCPOPTS_GEN_DHCP_TYPE(9)
#define DHCPLEASEQUERY        DHCPOPTS_GEN_DHCP_TYPE(10)
#define DHCPLEASEUNASSIGNED   DHCPOPTS_GEN_DHCP_TYPE(11)
#define DHCPLEASEUNKNOWN      DHCPOPTS_GEN_DHCP_TYPE(12)
#define DHCPLEASEACTIVE       DHCPOPTS_GEN_DHCP_TYPE(13)
#define DHCPBULKLEASEQUERY    DHCPOPTS_GEN_DHCP_TYPE(14)
#define DHCPLEASEQUERYDONE    DHCPOPTS_GEN_DHCP_TYPE(15)
#define DHCPACTIVELEASEQUERY  DHCPOPTS_GEN_DHCP_TYPE(16)
#define DHCPLEASEQUERYSTATUS  DHCPOPTS_GEN_DHCP_TYPE(17)
#define DHCPTLS               DHCPOPTS_GEN_DHCP_TYPE(18)

#define OPTS_END dhcpopt::makeDhcpOpt<255>()
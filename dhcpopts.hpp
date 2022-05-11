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
        static std::shared_ptr<dhcpopt> makeDhcpOpt() {
            auto opt = std::make_shared<dhcpopt>(code);
            opt->params = {opts...};
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

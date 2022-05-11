#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <tuple>
#include "buffer.hpp"

class udpsocketserver
{
#define SOCKADDR_IN_INIT {0, 0, 0, 0}
    int sock;
    static constexpr int siz = 65536;
    buffer buff{siz};
    sockaddr_in raddr = SOCKADDR_IN_INIT;

public:
    udpsocketserver() {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
        if(sock < 0)
            throw std::runtime_error("Cannot open socket: " + std::string(strerror(errno)));

        int opt = 1;
        if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("Cannot setopt for socket: " + std::string(strerror(errno)));

        opt = siz;
        if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0 )
            throw std::runtime_error("Cannot setopt for socket: " + std::string(strerror(errno)));

        opt = 1;
        if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Cannot setopt for socket: " + std::string(strerror(errno)));
        }

    }

    void bind(int port, std::string addr = "") {
        sockaddr_in saddr = SOCKADDR_IN_INIT;
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        
        int _addr = htonl(INADDR_ANY);
        if(!addr.empty())
           _addr = inet_addr(addr.c_str());
        saddr.sin_addr.s_addr = _addr;

        if(::bind(sock, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr)) < 0)
            throw std::runtime_error("Cannot bind socket: " + std::string(strerror(errno)));
    }

    std::tuple<buffer&, sockaddr_in&, int> receive() {
        socklen_t raddr_len = 0;
        buff.size = siz;

        int len = recvfrom(sock, buff.data(), buff.getSize(), 0, reinterpret_cast<sockaddr*>(&raddr), &raddr_len);
        if(len < 0)
            throw std::runtime_error("Recvfrom died with: " + std::string(strerror(errno)));

        return {buff, raddr, len};
    }

    bool send(const buffer& msg, const sockaddr_in &where) {
        int off = 0;
        while(int many = sendto(sock, msg.data() + off, msg.getSize() - off, 0, reinterpret_cast<const sockaddr*>(&where), sizeof(where)) < msg.getSize()) {
            if(many < 0)
                throw std::runtime_error("Sendto died with: " + std::string(strerror(errno)));
            else if(many == 0)
                return false;
            off += many;
        }
    }

    int getRawSock() const {
        return sock;
    }

    ~udpsocketserver() {
        close(sock);
    }

#undef SOCKADDR_IN_INIT
};
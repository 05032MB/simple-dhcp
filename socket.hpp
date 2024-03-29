#pragma once

#include "buffer.hpp"
#include "misc.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdexcept>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <utility>
#include <iostream>

class udpsocketserver
{
    int sock;
    static constexpr int siz = 65536;
    buffer buff{siz};
    sockaddr_in raddr = {};

public:
    udpsocketserver() {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(sock < 0)
            THROW_RUNTIME_GET_ERRNO("Cannot open socket");

        int opt = 1;
        if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0)
            THROW_RUNTIME_GET_ERRNO("Cannot setopt brd for socket");

        opt = siz;
        if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0 )
            THROW_RUNTIME_GET_ERRNO("Cannot setopt size for socket");

        socklen_t st = 0;
        getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &opt, &st);

        if(opt != siz)
            std::cout << "Warning: requested rcv buff: " << siz << " but got: " << opt << std::endl;
    }

    void bind(int port, const std::string& addr = "") {
        sockaddr_in saddr = {};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        
        int _addr = htonl(INADDR_ANY);
        if(!addr.empty()) {
            _addr = inet_addr(addr.c_str());
            if(_addr == -1) {
                std::cout << "Cannot bind the socket to the address, trying as an interface: " << addr << std::endl;
                // is iface name
                // https://stackoverflow.com/questions/1207746/problems-with-so-bindtodevice-linux-socket-option
                if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, addr.c_str(), addr.size()) < 0) {
                    THROW_RUNTIME_GET_ERRNO("Cannot bind the socket to the interface");
                }
            } else {
                saddr.sin_addr.s_addr = _addr;
            }
        }
        if(::bind(sock, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr)) < 0)
            THROW_RUNTIME_GET_ERRNO("Cannot bind socket");
    }

    std::pair<const buffer&, const sockaddr_in&> receive() {
        socklen_t raddr_len = 0;
        buff.size = siz;

        int len = recvfrom(sock, buff.data(), buff.getSize(), 0, reinterpret_cast<sockaddr*>(&raddr), &raddr_len);
        if(len < 0)
            THROW_RUNTIME_GET_ERRNO("Recvfrom died with: ");

        std::pair<const buffer&, const sockaddr_in&> rets{buff, raddr};
        
        buff.trimToSize(len);
        return rets;
    }

    bool send(const buffer& msg, const sockaddr_in &where) {
        int many = sendto(sock, msg.data(), msg.getSize(), 0, reinterpret_cast<const sockaddr*>(&where), sizeof(where));
        if(many < 0)
            THROW_RUNTIME_GET_ERRNO("Sendto died with: "); 
        return many == (int)msg.getSize();
    }

    int getRawSock() const {
        return sock;
    }

    ~udpsocketserver() {
        close(sock);
    }
};
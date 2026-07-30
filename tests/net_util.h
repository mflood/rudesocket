// net_util.h - shared plumbing for the rudesocket tests: a portable
// in-process TCP server (echo / silent) on 127.0.0.1 with an ephemeral port.
#ifndef RUDESOCKET_TESTS_NET_UTIL_H
#define RUDESOCKET_TESTS_NET_UTIL_H

#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET net_socket_t;
#define NET_INVALID_SOCKET INVALID_SOCKET
inline void net_close(net_socket_t s) { closesocket(s); }
inline void net_init()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
}
inline void net_cleanup() { WSACleanup(); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int net_socket_t;
#define NET_INVALID_SOCKET (-1)
inline void net_close(net_socket_t s) { ::close(s); }
inline void net_init() {}
inline void net_cleanup() {}
#endif

// Minimal TCP server bound to 127.0.0.1 on an ephemeral port.
class TestServer {
    net_socket_t d_listen;
    int d_port;

public:
    TestServer() : d_listen(NET_INVALID_SOCKET), d_port(0)
    {
        d_listen = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0; // ephemeral
        ::bind(d_listen, (sockaddr *)&addr, sizeof(addr));
        ::listen(d_listen, 1);
        socklen_t len = sizeof(addr);
        ::getsockname(d_listen, (sockaddr *)&addr, &len);
        d_port = ntohs(addr.sin_port);
    }

    ~TestServer()
    {
        if (d_listen != NET_INVALID_SOCKET)
            net_close(d_listen);
    }

    int port() const { return d_port; }

    // Accept one client and echo every byte back until it disconnects.
    void runEcho()
    {
        net_socket_t c = ::accept(d_listen, 0, 0);
        if (c == NET_INVALID_SOCKET)
            return;
        char buf[512];
        for (;;) {
            int rc = (int)::recv(c, buf, sizeof(buf), 0);
            if (rc <= 0)
                break;
            int off = 0;
            while (off < rc) {
                int sent = (int)::send(c, buf + off, rc - off, 0);
                if (sent <= 0) {
                    off = rc;
                    break;
                }
                off += sent;
            }
        }
        net_close(c);
    }

    // Accept one client, send 'data', then close immediately. Used to test
    // that readline()/reads() report EOF rather than an endless empty string.
    void runSendThenClose(const char *data)
    {
        net_socket_t c = ::accept(d_listen, 0, 0);
        if (c == NET_INVALID_SOCKET)
            return;
        int len = (int)std::strlen(data);
        int off = 0;
        while (off < len) {
            int sent = (int)::send(c, data + off, len - off, 0);
            if (sent <= 0)
                break;
            off += sent;
        }
        net_close(c);
    }

    // Accept one client, swallow its input, and never send anything back.
    void runSilent()
    {
        net_socket_t c = ::accept(d_listen, 0, 0);
        if (c == NET_INVALID_SOCKET)
            return;
        char buf[64];
        for (;;) {
            int rc = (int)::recv(c, buf, sizeof(buf), 0);
            if (rc <= 0)
                break;
        }
        net_close(c);
    }
};

#endif

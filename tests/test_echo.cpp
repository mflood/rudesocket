// Echo test: spawn an in-process TCP echo server on 127.0.0.1 (ephemeral
// port), connect with rude::Socket, send data, and verify readline()/read().
#include <rude/socket.h>

#include "net_util.h"

#include <cstdio>
#include <cstring>
#include <thread>

static int failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures; \
        } \
    } while (0)

int main()
{
    net_init();

    TestServer server;
    std::thread t(&TestServer::runEcho, &server);

    rude::Socket socket;
    socket.setTimeout(10, 0);
    CHECK(socket.connect("127.0.0.1", server.port()));

    // single line round trip
    CHECK(socket.sends("hello world\r\n"));
    const char *line = socket.readline();
    CHECK(line != 0);
    CHECK(line && std::strcmp(line, "hello world") == 0);

    // two lines flushed at once - readline() must split them
    CHECK(socket.sends("first line\r\nsecond line\r\n"));
    line = socket.readline();
    CHECK(line && std::strcmp(line, "first line") == 0);
    line = socket.readline();
    CHECK(line && std::strcmp(line, "second line") == 0);

    // raw send/read round trip
    CHECK(socket.send("abc", 3) == 3);
    char buf[3];
    int total = 0;
    while (total < 3) {
        int rc = socket.read(buf + total, 3 - total);
        CHECK(rc > 0);
        if (rc <= 0)
            break;
        total += rc;
    }
    CHECK(total == 3 && std::memcmp(buf, "abc", 3) == 0);

    CHECK(socket.close());
    t.join();
    net_cleanup();

    if (failures) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    std::printf("echo test OK\n");
    return 0;
}

// Timeout test: connect to a server that never responds and verify that
// reads with setTimeout(2, 0) fail in bounded time instead of hanging.
#include <rude/socket.h>

#include "net_util.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

static int failures = 0;

#define CHECK(cond)                                                              \
	do                                                                           \
	{                                                                            \
		if(!(cond))                                                              \
		{                                                                        \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                          \
		}                                                                        \
	} while(0)

static long secondsSince(std::chrono::steady_clock::time_point start)
{
	return (long) std::chrono::duration_cast<std::chrono::seconds>(
			   std::chrono::steady_clock::now() - start)
		.count();
}

int main()
{
	net_init();

	TestServer server;
	std::thread t(&TestServer::runSilent, &server);

	rude::Socket socket;
	socket.setTimeout(2, 0);
	CHECK(socket.connect("127.0.0.1", server.port()));

	// read() must fail after ~2s, not immediately and not hang
	auto start = std::chrono::steady_clock::now();
	char buf[16];
	int rc = socket.read(buf, 1);
	long elapsed = secondsSince(start);
	CHECK(rc < 0);
	CHECK(elapsed >= 1 && elapsed <= 30);
	CHECK(socket.getError()[0] != '\0');

	// readline() must time out the same way
	start = std::chrono::steady_clock::now();
	const char *line = socket.readline();
	elapsed = secondsSince(start);
	CHECK(line == 0);
	CHECK(elapsed >= 1 && elapsed <= 30);
	CHECK(socket.getError()[0] != '\0');

	socket.close();
	t.join();
	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	std::printf("timeout test OK\n");
	return 0;
}

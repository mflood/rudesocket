// Regression test for readline()/reads() at end of stream, fixed in 1.3.1.
//
// On peer close both returned d_readbuffer.c_str() unconditionally. With
// nothing buffered that is a non-NULL empty string, returned forever, so the
// documented loop
//
//     while ((line = socket.readline()) != NULL) { ... }
//
// never terminated once the peer went away - it span at full speed instead.
// EOF with nothing buffered is now reported as NULL.
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

int main()
{
	net_init();

	// ---- readline(): two complete lines, then EOF ----
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server,
					  "line one\r\nline two\r\n");

		rude::Socket socket;
		socket.setTimeout(10, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));

		const char *line = socket.readline();
		CHECK(line && std::strcmp(line, "line one") == 0);
		line = socket.readline();
		CHECK(line && std::strcmp(line, "line two") == 0);

		// Peer has closed and nothing is buffered: this must be NULL.
		CHECK(socket.readline() == 0);
		// ...and must stay NULL rather than alternating.
		CHECK(socket.readline() == 0);

		socket.close();
		t.join();
	}

	// ---- readline(): a final line with no terminator before close ----
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server,
					  "complete\r\ndangling");

		rude::Socket socket;
		socket.setTimeout(10, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));

		const char *line = socket.readline();
		CHECK(line && std::strcmp(line, "complete") == 0);

		// The unterminated tail is still delivered...
		line = socket.readline();
		CHECK(line && std::strcmp(line, "dangling") == 0);

		// ...and only then does EOF show up as NULL.
		CHECK(socket.readline() == 0);

		socket.close();
		t.join();
	}

	// ---- the documented loop must terminate ----
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server,
					  "a\r\nb\r\nc\r\n");

		rude::Socket socket;
		socket.setTimeout(10, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));

		const std::chrono::steady_clock::time_point start =
			std::chrono::steady_clock::now();

		int lines = 0;
		const char *line = 0;
		while((line = socket.readline()) != 0)
		{
			++lines;
			if(lines > 1000)
			{
				std::fprintf(stderr, "FAIL %s:%d: readline() loop did not "
									 "terminate at EOF\n",
							 __FILE__, __LINE__);
				++failures;
				break;
			}
		}

		const long elapsedms = (long) std::chrono::duration_cast<
								   std::chrono::milliseconds>(
								   std::chrono::steady_clock::now() - start)
								   .count();

		CHECK(lines == 3);
		// Terminating is the point; do it without spinning for seconds.
		CHECK(elapsedms < 5000);
		std::printf("readline loop read %d lines in %ld ms\n", lines, elapsedms);

		socket.close();
		t.join();
	}

	// ---- reads(): same contract at EOF ----
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server, "payload");

		rude::Socket socket;
		socket.setTimeout(10, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));

		const char *data = socket.reads();
		CHECK(data && std::strcmp(data, "payload") == 0);
		CHECK(socket.reads() == 0);

		socket.close();
		t.join();
	}

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	std::printf("readline eof test OK\n");
	return 0;
}

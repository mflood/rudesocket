// Writing to a peer that has gone must not kill the process.
//
// The default disposition of SIGPIPE is to terminate, so a send() to a socket
// whose peer has closed took the whole program down. Any peer could do it,
// deliberately or not, to any program built on this library -- and the program
// could not defend itself except by installing a global SIGPIPE handler, which
// is not a library's to require.
//
// It hid well: the first write after a clean close usually succeeds, because
// the peer's FIN only closes their direction. The kill comes on the write
// after the RST that follows. So the failure needs an exchange with at least
// two writes past the close, which is why it surfaced when the rudesmtp tests
// started scripting servers that hang up mid-conversation, and never before.
//
// Fixed with SO_NOSIGPIPE on BSD/macOS and MSG_NOSIGNAL per send on Linux.
// Windows has no SIGPIPE.
//
// This test does NOT install a SIGPIPE handler: doing so would mask exactly
// what it is here to catch. If the library regresses, the process dies with
// signal 13 and ctest reports it.

#include "net_util.h"

#include <rude/socket.h>

#include <cstdio>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

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
	// Unbuffered: if the library regresses, this process is killed by a
	// signal and anything still sitting in the buffer is lost, which makes
	// the failure much harder to place.
	setvbuf(stdout, 0, _IONBF, 0);

	net_init();

	// The server greets and hangs up. Everything the client sends after
	// that is a write to a departed peer.
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server, "220 goodbye\n");

		rude::Socket s;
		s.setTimeout(5, 0);
		CHECK(s.connect("127.0.0.1", server.port()));

		const char *greeting = s.readline();
		CHECK(greeting != 0);
		std::printf("read \"%s\", peer has now closed\n", greeting ? greeting : "");

		t.join();

		// Repeated so the test crosses the FIN-then-RST boundary rather
		// than stopping at the first write, which typically succeeds.
		for(int i = 0; i < 10; i++)
		{
			s.sends("PING\r\n");
		}

		std::printf("survived 10 sends to a closed peer\n");
		s.close();
	}

	// Same again with the per-byte send path, which a timeout selects.
	// It calls ::send() once per character, so a long enough string gives
	// the RST plenty of chances to arrive mid-write.
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server, "220 goodbye\n");

		rude::Socket s;
		s.setTimeout(2, 0);
		CHECK(s.connect("127.0.0.1", server.port()));
		s.readline();
		t.join();

		for(int i = 0; i < 5; i++)
		{
			s.sends("PING PING PING PING PING PING PING PING\r\n");
		}

		std::printf("survived the per-byte send path too\n");
		s.close();
	}

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("sigpipe OK\n");
	return 0;
}

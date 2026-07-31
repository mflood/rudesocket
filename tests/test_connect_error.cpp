// Regression test for connect() failure reporting.
//
// socket.h documents getError() as "a description of the last known error".
// With a timeout configured, a refused connection described itself as
// "Operation now in progress" instead: connecttimeout() puts the descriptor in
// non-blocking mode, so ::connect() returns EINPROGRESS immediately and the
// real outcome lands only in the socket's SO_ERROR. That value was read,
// compared against zero, and thrown away -- leaving errno still holding
// EINPROGRESS when the caller reached strerror(). Without a timeout the same
// failure reported "Connection refused" correctly, so whether you got a usable
// diagnosis depended on an unrelated setting.
//
// The same failure path also leaked the descriptor it had opened.

#include "net_util.h"

#include <rude/socket.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

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

static std::string lower(const std::string &s)
{
	std::string out(s);
	for(size_t i = 0; i < out.size(); i++)
	{
		out[i] = (char) std::tolower((unsigned char) out[i]);
	}
	return out;
}

// Connects to a port nothing is listening on and returns getError().
static std::string refusalError(int port, bool withTimeout)
{
	rude::Socket s;
	if(withTimeout)
	{
		s.setTimeout(5, 0);
	}
	const bool connected = s.connect("127.0.0.1", port);
	CHECK(!connected);
	const char *err = s.getError();
	CHECK(err != 0);
	return err ? err : "";
}

int main()
{
	net_init();

	const int port = net_dead_port();
	if(port == 0)
	{
		std::fprintf(stderr, "could not reserve a dead port\n");
		return 1;
	}

	const std::string timed = refusalError(port, true);
	const std::string untimed = refusalError(port, false);

	std::printf("with timeout:    \"%s\"\n", timed.c_str());
	std::printf("without timeout: \"%s\"\n", untimed.c_str());

	// The bug itself: EINPROGRESS is an intermediate state of a non-blocking
	// connect, never a reason one failed.
	CHECK(lower(timed).find("in progress") == std::string::npos);

	// Every platform words it differently -- "Connection refused" on Linux and
	// macOS, "...the target machine actively refused it" on Windows -- but all
	// three name the refusal.
	CHECK(lower(timed).find("refused") != std::string::npos);
	CHECK(lower(untimed).find("refused") != std::string::npos);

	// Configuring a timeout must not change how a non-timeout failure is
	// described. This is the invariant that was broken.
	CHECK(timed == untimed);

#ifndef _WIN32
	// A failed connect used to leave its descriptor open, and nothing could
	// reclaim it: close() refuses to act because the comm object was never
	// bound. The lowest free descriptor number stands in for the count.
	{
		net_socket_t probe = ::socket(AF_INET, SOCK_STREAM, 0);
		net_close(probe);
		const int before = (int) probe;

		for(int i = 0; i < 24; i++)
		{
			rude::Socket s;
			s.setTimeout(5, 0);
			s.connect("127.0.0.1", port);
		}

		probe = ::socket(AF_INET, SOCK_STREAM, 0);
		net_close(probe);
		const int after = (int) probe;

		std::printf("lowest free fd: %d before, %d after 24 failed connects\n",
					before, after);
		CHECK(after == before);
	}
#endif

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("connect error reporting OK\n");
	return 0;
}

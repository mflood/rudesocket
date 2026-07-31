// Regression test for setTimeout() applied to connect().
//
// socket.h: "Sets the timeout value for Connect, Read and Send operations."
// On Windows that covered two of the three. The non-blocking machinery in
// Socket_Connect_Normal::connecttimeout() sat inside #ifndef WIN32, so what
// ran there was a plain blocking ::connect() with msec ignored -- a connect to
// an address that silently drops packets waited out the stack's own SYN-retry
// period, roughly 21 seconds, regardless of what the caller asked for.
//
// What this asserts is that connect() is *bounded*, which is the contract.
// Proving the exact timeout would need an address guaranteed to blackhole, and
// no such thing exists across every network a CI runner might sit on:
// 192.0.2.1 is reserved for documentation (RFC 5737) and should route nowhere,
// but a network that answers with an ICMP unreachable makes the connect fail
// promptly instead. That outcome is also fine -- it is not a hang either --
// so the test accepts it and says so in its output rather than failing on a
// network it cannot control. It never passes vacuously in the case that
// matters: an unbounded connect blows the deadline.

#include "net_util.h"

#include <rude/socket.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

// Reserved for documentation; should not be routable from anywhere.
static const char *BLACKHOLE = "192.0.2.1";

static const int TIMEOUT_SECONDS = 2;

// Comfortably above the timeout, comfortably below the ~21s an unbounded
// connect takes on Windows and the ~75s it takes on Linux.
static const long DEADLINE_MS = 10000;

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

	rude::Socket s;
	s.setTimeout(TIMEOUT_SECONDS, 0);

	const std::chrono::steady_clock::time_point start =
		std::chrono::steady_clock::now();

	const bool connected = s.connect(BLACKHOLE, 80);

	const long ms = (long) std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::steady_clock::now() - start)
						.count();

	const std::string err = s.getError() ? s.getError() : "";

	std::printf("connect(%s) returned %d after %ld ms: \"%s\"\n",
				BLACKHOLE, (int) connected, ms, err.c_str());

	// Nothing is listening there, whatever the network does about it.
	CHECK(!connected);

	// The contract: the call is bounded by what setTimeout() was given.
	CHECK(ms < DEADLINE_MS);

	if(err.find("Timed Out") != std::string::npos)
	{
		// The address blackholed, so the timeout is what ended the wait. It
		// must have actually waited -- giving up early would cut short a peer
		// that is merely slow to answer.
		CHECK(ms >= (TIMEOUT_SECONDS * 1000) - 500);
		std::printf("timed out as expected\n");
	}
	else
	{
		// The network answered for the address. Still bounded, still not a
		// hang, but the timeout itself went unexercised on this runner.
		std::printf("NOTE: the network rejected %s outright, so the timeout "
					"path was not exercised here -- only boundedness was\n",
					BLACKHOLE);
	}

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("connect timeout OK\n");
	return 0;
}

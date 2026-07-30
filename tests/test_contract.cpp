// Contract tests: assertions come from what socket.h promises, plus the
// defensive behaviour a caller is entitled to expect when a documented
// precondition ("a connection must be established") is not met.
//
// socket.h is thinner on stated contracts than the other two libraries, so
// this leans on the two things it does promise everywhere - that getError()
// describes the last error, and that failures are reported rather than
// signalled by a crash.
#include <rude/socket.h>

#include "net_util.h"

#include <cstdio>
#include <cstring>
#include <string>
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

// A port nothing is listening on: bind one, learn the port, then drop it.
static int deadPort()
{
	TestServer probe;
	return probe.port();
}

int main()
{
	net_init();

	// ---- getError() is always a readable string, never NULL -------------
	{
		rude::Socket socket;
		CHECK(socket.getError() != 0);
	}

	// ---- operations before a connection exists must not crash -----------
	// socket.h states a connection must be established first. That is a
	// precondition, not a licence to segfault: a CGI or daemon that gets the
	// order wrong should get an error, not a crash.
	{
		rude::Socket socket;

		char buffer[64];
		CHECK(socket.read(buffer, (int) sizeof(buffer)) <= 0);
		CHECK(socket.send("x", 1) <= 0);
		CHECK(!socket.sends("x"));

		// These may report emptiness however they like, but must return.
		socket.reads();
		socket.readline();

		CHECK(socket.getError() != 0);
	}

	// ---- close() without a connection, and twice, must not crash --------
	{
		rude::Socket socket;
		socket.close();
		socket.close();
		CHECK(socket.getError() != 0);
	}

	// ---- sends(NULL) is rejected, with an error ------------------------
	{
		TestServer server;
		std::thread t(&TestServer::runEcho, &server);

		rude::Socket socket;
		socket.setTimeout(5, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));
		CHECK(!socket.sends(0));
		CHECK(socket.getError() != 0);
		CHECK(socket.getError()[0] != '\0');
		socket.close();
		t.join();
	}

	// ---- a refused connection returns false and describes itself --------
	{
		rude::Socket socket;
		socket.setTimeout(5, 0);
		const int port = deadPort();
		CHECK(!socket.connect("127.0.0.1", port));
		CHECK(socket.getError() != 0);
		CHECK(socket.getError()[0] != '\0');
		std::printf("refused connect error: %s\n", socket.getError());
	}

	// ---- an unresolvable host returns false and describes itself --------
	{
		rude::Socket socket;
		socket.setTimeout(5, 0);
		CHECK(!socket.connect("no-such-host.invalid", 80));
		CHECK(socket.getError() != 0);
		CHECK(socket.getError()[0] != '\0');
		std::printf("bad host error: %s\n", socket.getError());
	}

	// ---- connect(NULL) must be rejected, not dereferenced ---------------
	{
		rude::Socket socket;
		socket.setTimeout(5, 0);
		CHECK(!socket.connect(0, 80));
		CHECK(socket.getError() != 0);
	}

	// ---- a successful round trip, then reuse of the same object ---------
	{
		TestServer server;
		std::thread t(&TestServer::runEcho, &server);

		rude::Socket socket;
		socket.setTimeout(5, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));
		CHECK(socket.sends("ping\r\n"));

		const char *line = socket.readline();
		CHECK(line != 0 && std::strcmp(line, "ping") == 0);

		CHECK(socket.close());
		t.join();

		// The same object connects again to a fresh server.
		TestServer server2;
		std::thread t2(&TestServer::runEcho, &server2);
		CHECK(socket.connect("127.0.0.1", server2.port()));
		CHECK(socket.sends("pong\r\n"));
		line = socket.readline();
		CHECK(line != 0 && std::strcmp(line, "pong") == 0);
		socket.close();
		t2.join();
	}

	// ---- setTimeout(0, 0) is accepted and leaves the socket usable ------
	// "Setting the timeout to 0 removes the timeout - making the Socket
	// blocking." The echo peer always answers, so this must not hang.
	{
		TestServer server;
		std::thread t(&TestServer::runEcho, &server);

		rude::Socket socket;
		socket.setTimeout(0, 0);
		CHECK(socket.connect("127.0.0.1", server.port()));
		CHECK(socket.sends("blocking\r\n"));
		const char *line = socket.readline();
		CHECK(line != 0 && std::strcmp(line, "blocking") == 0);
		socket.close();
		t.join();
	}

#ifdef RUDESOCKET_WITH_SSL
	// ---- connectSSL against a plain TCP peer fails cleanly --------------
	// The handshake cannot succeed; what matters is that it fails, says so,
	// and does not hang or crash.
	{
		TestServer server;
		std::thread t(&TestServer::runSilent, &server);

		rude::Socket socket;
		socket.setTimeout(5, 0);
		CHECK(!socket.connectSSL("127.0.0.1", server.port()));
		CHECK(socket.getError() != 0);
		CHECK(socket.getError()[0] != '\0');
		std::printf("plain-peer SSL error: %s\n", socket.getError());
		socket.close();
		t.join();
	}
#else
	std::printf("SKIP: built without SSL, connectSSL contract not exercised\n");
#endif

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d contract check(s) failed\n", failures);
		return 1;
	}
	std::printf("contract OK\n");
	return 0;
}

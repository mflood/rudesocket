// SSL test: generate a self-signed certificate, run `openssl s_server` as a
// subprocess, and verify:
//   1. connectSSL() with verification opted out can talk to it, and that
//      readline() drains data buffered inside OpenSSL (SSL_pending) - two
//      lines delivered in one TLS record must both be readable (this used
//      to deadlock in select()).
//   2. connectSSL() with verification ON (the default) FAILS against the
//      self-signed certificate and getError() is non-empty.
//
// Skips cleanly (exit 0) on Windows and when the openssl CLI is unavailable.
#ifdef _WIN32
#include <cstdio>
int main()
{
	std::printf("SKIP: SSL subprocess test not supported on Windows\n");
	return 0;
}
#else

#include <rude/socket.h>

#include "net_util.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

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

// Wait (up to ~10s) until something is listening on 127.0.0.1:port.
// NOTE: each successful probe consumes one accept, so the s_server
// instances below run with -naccept 2 (probe + actual test connection).
static bool waitForPort(int port)
{
	for(int i = 0; i < 100; ++i)
	{
		net_socket_t s = ::socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons((unsigned short) port);
		int rc = ::connect(s, (sockaddr *) &addr, sizeof(addr));
		net_close(s);
		if(rc == 0)
			return true;
		usleep(100000);
	}
	return false;
}

static FILE *startServer(int port, const char *logfile)
{
	char cmd[512];
	std::snprintf(cmd, sizeof(cmd),
				  "openssl s_server -quiet -naccept 2 -accept %d"
				  " -key key.pem -cert cert.pem > %s 2>&1",
				  port, logfile);
	return popen(cmd, "w"); // we write lines into s_server's stdin
}

int main()
{
	if(std::system("openssl version > /dev/null 2>&1") != 0)
	{
		std::printf("SKIP: openssl CLI not available\n");
		return 0;
	}

	// self-signed certificate for CN=localhost
	CHECK(std::system("openssl req -x509 -newkey rsa:2048 -keyout key.pem"
					  " -out cert.pem -days 2 -nodes -subj /CN=localhost"
					  " > /dev/null 2>&1") == 0);

	int port1 = 20000 + (int) (getpid() % 20000);
	int port2 = port1 + 1;

	// ---- Part 1: verification opted out; data flow and SSL_pending ----
	{
		FILE *server = startServer(port1, "server1.log");
		CHECK(server != 0);
		CHECK(waitForPort(port1));

		rude::Socket socket;
		socket.setTimeout(10, 0);
		socket.setSSLVerify(false); // self-signed test certificate
		CHECK(socket.connectSSL("localhost", port1));
		if(!failures)
		{
			CHECK(socket.sends("hello over tls\r\n"));

			// Two lines written into s_server's stdin in one flush arrive in
			// one TLS record: the second readline() must be served from
			// OpenSSL's internal buffer (SSL_pending), not from select().
			std::fputs("line one\r\nline two\r\n", server);
			std::fflush(server);

			const char *line = socket.readline();
			CHECK(line && std::strcmp(line, "line one") == 0);
			line = socket.readline();
			CHECK(line && std::strcmp(line, "line two") == 0);

			socket.close();
		}
		else
		{
			std::fprintf(stderr, "connectSSL failed: %s\n", socket.getError());
		}
		if(server)
			pclose(server);
	}

	// ---- Part 2: verification ON (default) must reject the self-signed cert ----
	{
		FILE *server = startServer(port2, "server2.log");
		CHECK(server != 0);
		CHECK(waitForPort(port2));

		rude::Socket socket;
		socket.setTimeout(10, 0);
		// no setSSLVerify() call: verification is on by default
		CHECK(!socket.connectSSL("127.0.0.1", port2));
		CHECK(socket.getError()[0] != '\0');
		std::printf("verify-on error (expected): %s\n", socket.getError());
		socket.close();
		if(server)
			pclose(server);
	}

	if(failures)
	{
		std::fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	std::printf("ssl test OK\n");
	return 0;
}
#endif

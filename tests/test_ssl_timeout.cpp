// Regression test for setTimeout() over SSL, fixed in 1.3.1.
//
// 1.3.0's NEWS claimed timeouts were honoured on SSL reads. They were not.
// virtualread() did select()-then-SSL_read(), but the socket is blocking once
// connect() finishes, so against a TLS 1.3 server the post-handshake
// NewSessionTicket record made the socket readable, SSL_read() consumed it,
// found no application data behind it, and then blocked in the kernel - past
// the deadline, indefinitely. The read only returned when the peer eventually
// sent something or went away.
//
// The test connects to an `openssl s_server` that never sends application
// data, with session tickets left at their default (this is what makes the
// original bug reproduce), and requires the read to fail on schedule.
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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
// Each successful probe consumes one accept, so s_server runs with -naccept 2.
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

static long elapsedMsSince(const std::chrono::steady_clock::time_point &start)
{
	return (long) std::chrono::duration_cast<std::chrono::milliseconds>(
			   std::chrono::steady_clock::now() - start)
		.count();
}

int main()
{
	if(std::system("openssl version > /dev/null 2>&1") != 0)
	{
		std::printf("SKIP: openssl CLI not available\n");
		return 0;
	}

	CHECK(std::system("openssl req -x509 -newkey rsa:2048 -keyout key.pem"
					  " -out cert.pem -days 2 -nodes -subj /CN=localhost"
					  " > /dev/null 2>&1") == 0);

	const int port = 40000 + (int) (getpid() % 20000);

	// Session tickets deliberately left at the default: a TLS 1.3
	// NewSessionTicket arriving after the handshake is exactly what used to
	// satisfy select() and then wedge SSL_read() in the kernel.
	char cmd[512];
	std::snprintf(cmd, sizeof(cmd),
				  "openssl s_server -quiet -naccept 2 -accept %d"
				  " -key key.pem -cert cert.pem > server.log 2>&1",
				  port);
	FILE *server = popen(cmd, "w");
	CHECK(server != 0);
	CHECK(waitForPort(port));

	rude::Socket socket;
	socket.setSSLVerify(false); // self-signed test certificate
	socket.setTimeout(2, 0);

	CHECK(socket.connectSSL("localhost", port));
	if(failures)
	{
		std::fprintf(stderr, "connectSSL failed: %s\n", socket.getError());
		if(server)
			pclose(server);
		return 1;
	}

	// Give the server something to receive, so the only thing outstanding is
	// our read. s_server prints it to its stdout; it never replies.
	CHECK(socket.sends("GET / HTTP/1.0\r\n\r\n"));

	// The server will not send application data. readline() must give up
	// after roughly the configured 2s, not block until the peer goes away.
	{
		const std::chrono::steady_clock::time_point start =
			std::chrono::steady_clock::now();
		const char *line = socket.readline();
		const long ms = elapsedMsSince(start);

		CHECK(line == 0);
		CHECK(ms >= 1500); // it really waited
		CHECK(ms < 12000); // ...but nothing like forever
		CHECK(socket.getError()[0] != '\0');
		std::printf("readline timed out after %ld ms: %s\n", ms, socket.getError());
	}

	// Same again with read(), and a shorter timeout, to show the deadline is
	// taken from setTimeout() rather than being a fixed value.
	{
		socket.setTimeout(1, 0);
		char buffer[128];
		const std::chrono::steady_clock::time_point start =
			std::chrono::steady_clock::now();
		int rc = socket.read(buffer, (int) sizeof(buffer));
		const long ms = elapsedMsSince(start);

		CHECK(rc < 0);
		CHECK(ms < 8000);
		CHECK(socket.getError()[0] != '\0');
		std::printf("read timed out after %ld ms: %s\n", ms, socket.getError());
	}

	socket.close();
	if(server)
		pclose(server);

	if(failures)
	{
		std::fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	std::printf("ssl timeout test OK\n");
	return 0;
}
#endif

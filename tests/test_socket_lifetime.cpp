// A Socket must release its connection when it is destroyed.
//
// Nothing in the teardown path closed anything: ~Socket deleted the
// implementation, ~Socket_TCPClient deleted the connection and comm objects,
// and Socket_Comm's destructor deliberately does not close (its comment says
// children should call finish(), and finish() only unbinds). So every Socket
// that went out of scope without an explicit close() leaked its descriptor,
// and the peer was left holding a connection that would only go away when the
// process did. A program opening sockets in a loop ran out of descriptors.
//
// The descriptor count is read from /dev/fd rather than by watching the
// number a fresh socket() is handed: that reports the LOWEST FREE descriptor,
// which only moves when the leak starts at the bottom of the table, and the
// test server here opens and closes its own sockets in between.

#include "net_util.h"

#include <rude/socket.h>

#include <cstdio>
#include <thread>

#ifndef _WIN32
#include <dirent.h>
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

static int openFdCount()
{
#ifdef _WIN32
	return -1;
#else
	DIR *dir = opendir("/dev/fd");
	if(!dir)
	{
		return -1;
	}
	int n = 0;
	while(readdir(dir))
	{
		n++;
	}
	closedir(dir);
	return n;
#endif
}

// One connect, optionally closed explicitly, then destroyed.
static void connectOnce(bool explicitClose)
{
	TestServer server;
	std::thread t(&TestServer::runSendThenClose, &server, "hello\n");
	{
		rude::Socket s;
		s.setTimeout(5, 0);
		if(!s.connect("127.0.0.1", server.port()))
		{
			std::fprintf(stderr, "connect failed: %s\n", s.getError());
		}
		s.readline();
		if(explicitClose)
		{
			s.close();
		}
	}
	t.join();
}

int main()
{
	net_init();

	// Warm up once so any one-time allocation is not counted as a leak.
	connectOnce(true);

	// ---- destroyed without close() ---------------------------------------
	{
		const int before = openFdCount();
		for(int i = 0; i < 12; i++)
		{
			connectOnce(false);
		}
		const int after = openFdCount();
		std::printf("no explicit close: %d descriptors before, %d after 12 sockets\n",
					before, after);
		if(before >= 0 && after >= 0)
		{
			CHECK(after == before);
		}
	}

	// ---- closed explicitly, then destroyed -------------------------------
	//
	// The destructor now closes too, so this is the path that would double
	// close if close() did not forget the descriptor.
	{
		const int before = openFdCount();
		for(int i = 0; i < 12; i++)
		{
			connectOnce(true);
		}
		const int after = openFdCount();
		std::printf("explicit close: %d descriptors before, %d after 12 sockets\n",
					before, after);
		if(before >= 0 && after >= 0)
		{
			CHECK(after == before);
		}
	}

	// ---- close() is idempotent -------------------------------------------
	//
	// socket.h: "Safe to call more than once, and safe to call on a Socket
	// that was never connected".
	{
		TestServer server;
		std::thread t(&TestServer::runSendThenClose, &server, "hello\n");
		{
			rude::Socket s;
			s.setTimeout(5, 0);
			CHECK(s.connect("127.0.0.1", server.port()));
			CHECK(s.close());
			CHECK(s.close()); // succeeds, does nothing
			CHECK(s.close());
		}
		t.join();
		std::printf("close() is repeatable\n");
	}
	{
		rude::Socket s; // never connected
		CHECK(s.close());
		CHECK(s.getError() != 0);
		std::printf("close() on an unconnected Socket succeeds\n");
	}

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("socket lifetime OK\n");
	return 0;
}

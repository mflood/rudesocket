// demo.cpp - plain TCP HTTP request to example.com, plus (when built with
// SSL support) an HTTPS request with certificate verification enabled -
// the HTTPS part proves SNI + verification + modern TLS end to end.
#include <rude/socket.h>

#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif

int main()
{
#ifdef _WIN32
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

	// --- Plain TCP: HTTP GET ---
	{
		rude::Socket socket;
		socket.setTimeout(10, 0);
		if(!socket.connect("example.com", 80))
		{
			std::cerr << "connect failed: " << socket.getError() << "\n";
			return 1;
		}
		socket.sends("GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n");
		const char *status = socket.readline();
		if(!status)
		{
			std::cerr << "read failed: " << socket.getError() << "\n";
			return 1;
		}
		std::cout << "[http]  " << status << "\n";
		socket.close();
	}

#ifdef RUDESOCKET_WITH_SSL
	// --- HTTPS GET with certificate verification (the default) ---
	{
		rude::Socket socket;
		socket.setTimeout(10, 0);
		// Certificate verification is ON by default: the connection succeeds
		// only if example.com presents a valid certificate for its hostname.
		if(!socket.connectSSL("example.com", 443))
		{
			std::cerr << "connectSSL failed: " << socket.getError() << "\n";
			return 1;
		}
		socket.sends("GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n");
		const char *status = socket.readline();
		if(!status)
		{
			std::cerr << "read failed: " << socket.getError() << "\n";
			return 1;
		}
		std::cout << "[https] " << status << " (certificate verified)\n";
		socket.close();
	}
#else
	std::cout << "[https] skipped - built without SSL support\n";
#endif

#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}

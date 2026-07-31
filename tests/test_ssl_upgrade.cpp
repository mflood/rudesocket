// Test for startSSL(), which negotiates TLS over a connection already open.
//
// connectSSL() establishes TLS as part of connecting, which is the wrong shape
// for SMTP, IMAP, POP3 and FTP: those open in the clear, read a greeting, ask
// for encryption with a command, and only then upgrade. By the time they know
// they want TLS the connection exists, so there was previously no way to get
// it -- rudesocket could only offer TLS at connect time.
//
// The server here is deliberately SMTP-shaped (see tls_server.h). The test
// covers:
//
//   1. The upgrade works: plaintext greeting, plaintext command, then an
//      encrypted exchange the server confirms it received over TLS.
//   2. Verification is honoured. The certificate is self-signed, so the
//      default (verify on) must REFUSE it -- an upgrade that silently skipped
//      verification would be worse than no upgrade, since the caller would
//      believe the channel was authenticated.
//   3. A failed upgrade does not leak the descriptor. There is no way for the
//      caller to close it: close() goes through the comm object, which a
//      failed handshake leaves unbound.
//   4. Upgrading twice is refused rather than nesting one TLS session inside
//      another.
//
// Needs the openssl CLI to generate a self-signed certificate; skips cleanly
// without it, as test_ssl.cpp does.

#include "net_util.h"
#include "tls_server.h"

#include <rude/socket.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#ifndef _WIN32
#include <dirent.h>
#endif

// Number of descriptors this process has open.
//
// The obvious cheaper probe -- open a socket, close it, and watch the number
// it was given -- reports the LOWEST free descriptor, which only moves if the
// leak happens to start at the bottom of the table. The test server here opens
// and closes its own sockets between iterations, so leaked descriptors sit in
// the middle and that probe reads clean while descriptors pile up. Count them
// instead.
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

// Drives the plaintext half of the dialogue, up to the point of upgrade.
static bool openAndRequestUpgrade(rude::Socket &s, int port)
{
	if(!s.connect("127.0.0.1", port))
	{
		std::fprintf(stderr, "connect failed: %s\n", s.getError());
		return false;
	}
	const char *greeting = s.readline(); // 220 test ready, in the clear
	if(!greeting || std::strncmp(greeting, "220", 3) != 0)
	{
		std::fprintf(stderr, "unexpected greeting\n");
		return false;
	}
	s.sends("STARTTLS\r\n");
	const char *go = s.readline(); // 220 go ahead, still in the clear
	return go && std::strncmp(go, "220", 3) == 0;
}

int main()
{
	if(std::system("openssl version > /dev/null 2>&1") != 0)
	{
		std::printf("SKIP: openssl CLI not available\n");
		return 0;
	}
	if(std::system("openssl req -x509 -newkey rsa:2048 -nodes -keyout key.pem"
				   " -out cert.pem -days 1 -subj /CN=127.0.0.1"
				   " > /dev/null 2>&1") != 0)
	{
		std::printf("SKIP: could not generate a test certificate\n");
		return 0;
	}

	net_init();
	SSL_library_init();

	// 1. The upgrade itself, with verification opted out for the self-signed
	//    certificate.
	{
		StartTlsServer server;
		std::thread t(&StartTlsServer::run, &server);

		rude::Socket s;
		s.setSSLVerify(false);

		if(openAndRequestUpgrade(s, server.port()))
		{
			const bool upgraded = s.startSSL();
			CHECK(upgraded);
			if(!upgraded)
			{
				std::fprintf(stderr, "startSSL failed: %s\n", s.getError());
			}
			else
			{
				s.sends("PING\r\n");
				const char *pong = s.readline();
				CHECK(pong != 0);
				CHECK(pong && std::strcmp(pong, "PONG") == 0);
				s.close();
			}
		}
		else
		{
			CHECK(false);
		}

		t.join();

		// The server's view: it saw the request in the clear, completed a
		// handshake, and read the follow-up through TLS.
		CHECK(server.plaintextLine() == "STARTTLS");
		CHECK(server.handshook());
		CHECK(server.encryptedLine() == "PING");

		std::printf("upgraded: plaintext \"%s\", handshake %s, encrypted \"%s\"\n",
					server.plaintextLine().c_str(),
					server.handshook() ? "ok" : "FAILED",
					server.encryptedLine().c_str());
	}

	// 2. With verification left at its default, the self-signed certificate
	//    must be rejected. 3. And the failure must not leak the descriptor.
	{
		const int before = openFdCount();

		for(int i = 0; i < 8; i++)
		{
			StartTlsServer server;
			server.expectHandshakeFailure();
			std::thread t(&StartTlsServer::run, &server);

			rude::Socket s; // verification ON, the default

			if(openAndRequestUpgrade(s, server.port()))
			{
				const bool upgraded = s.startSSL();
				CHECK(!upgraded);
				if(i == 0)
				{
					std::printf("verification on, self-signed cert: "
								"startSSL()=%d, error \"%s\"\n",
								(int) upgraded, s.getError());
					CHECK(std::strlen(s.getError()) > 0);
				}
			}
			else
			{
				CHECK(false);
			}
			t.join();
		}

		const int after = openFdCount();

		std::printf("open descriptors: %d before, %d after 8 refused upgrades\n",
					before, after);
		if(before >= 0 && after >= 0)
		{
			CHECK(after == before);
		}
	}

	// 4. A second upgrade must be refused, not nested inside the first.
	{
		StartTlsServer server;
		std::thread t(&StartTlsServer::run, &server);

		rude::Socket s;
		s.setSSLVerify(false);

		if(openAndRequestUpgrade(s, server.port()))
		{
			CHECK(s.startSSL());
			const bool again = s.startSSL();
			CHECK(!again);
			std::printf("second startSSL() on the same connection: %d, \"%s\"\n",
						(int) again, s.getError());
			s.sends("PING\r\n");
			s.readline();
			s.close();
		}
		else
		{
			CHECK(false);
		}
		t.join();
	}

	net_cleanup();

	if(failures)
	{
		std::fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("TLS upgrade OK\n");
	return 0;
}

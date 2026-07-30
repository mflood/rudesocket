// socket_comm_ssh.cpp
//
// Copyright (C) 2001, 2002, 2003, 2004, 2005 Matthew Flood
// See file AUTHORS for contact information
//
// This file is part of RudeSocket.
//
// RudeSocket is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// RudeSocket is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RudeSocket; (see COPYING) if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//------------------------------------------------------------------------

#ifdef USING_OPENSSL

#include "socket_comm_ssh.h"

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

#ifndef INCLUDED_STRING_H
#include <string.h>
#define INCLUDED_STRING_H
#endif

#ifndef INCLUDED_STDIO_H
#include <stdio.h>
#define INCLUDED_STDIO_H
#endif

#ifndef INCLUDED_CHRONO
#include <chrono>
#define INCLUDED_CHRONO
#endif

#ifndef WIN32
#ifndef INCLUDED_FCNTL_H
#include <fcntl.h>
#define INCLUDED_FCNTL_H
#endif
#endif

using namespace std;

namespace rude{
namespace sckt{

//=
// Returns true if 'host' looks like an IPv4 or IPv6 address literal.
// SNI must not be sent for IP literals (RFC 6066 section 3), and
// hostname verification does not apply to them either.
//=
static bool isIPLiteral(const char *host)
{
	if(strchr(host, ':'))
	{
		// IPv6 literal
		//
		return true;
	}
	size_t span = strspn(host, "0123456789.");
	return span > 0 && host[span] == '\0';
}

typedef std::chrono::steady_clock SocketClock;

//=
// Time remaining until 'deadline', as a timeval for select().
// Returns false once the deadline has passed.
//=
static bool remainingUntil(const SocketClock::time_point &deadline, struct timeval *tv)
{
	SocketClock::duration left = deadline - SocketClock::now();
	if(left.count() <= 0)
	{
		return false;
	}
	std::chrono::microseconds usecs = std::chrono::duration_cast<std::chrono::microseconds>(left);
	tv->tv_sec = (long) (usecs.count() / 1000000);
	tv->tv_usec = (long) (usecs.count() % 1000000);
	return true;
}

//=
// Puts the socket into non-blocking mode for the lifetime of the object and
// restores the previous mode on destruction.
//
// This is what makes setTimeout() work over SSL.  The socket is blocking
// once connect() finishes, so select()-then-SSL_read() is not enough: a
// post-handshake record such as a TLS 1.3 NewSessionTicket makes the socket
// readable, SSL_read() consumes it, finds no application data behind it, and
// then blocks in the kernel waiting for more - past the deadline, forever.
// With a non-blocking socket SSL_read() returns SSL_ERROR_WANT_READ instead
// and the caller can go back to select() with the remaining time.
//=
class NonBlockingScope
{
public:
	explicit NonBlockingScope(SOCKET sock)
		: d_socket(sock), d_restore(false)
#ifndef WIN32
		, d_oldflags(0)
#endif
	{
#ifdef WIN32
		u_long mode = 1;
		if(ioctlsocket(d_socket, FIONBIO, &mode) == 0)
		{
			d_restore = true;
		}
#else
		d_oldflags = fcntl(d_socket, F_GETFL, 0);
		if(d_oldflags != -1 && fcntl(d_socket, F_SETFL, d_oldflags | O_NONBLOCK) != -1)
		{
			d_restore = true;
		}
#endif
	}

	~NonBlockingScope()
	{
		if(!d_restore)
		{
			return;
		}
#ifdef WIN32
		u_long mode = 0;
		ioctlsocket(d_socket, FIONBIO, &mode);
#else
		fcntl(d_socket, F_SETFL, d_oldflags);
#endif
	}

	// True if the socket really is non-blocking now.
	//
	bool ok() const { return d_restore; }

private:
	NonBlockingScope(const NonBlockingScope &);
	NonBlockingScope &operator=(const NonBlockingScope &);

	SOCKET d_socket;
	bool d_restore;
#ifndef WIN32
	int d_oldflags;
#endif
};

Socket_Comm_SSH::Socket_Comm_SSH()
{
	ctx = (SSL_CTX*) 0;
	ssl = (SSL*) 0;
	d_hostname = "";
	d_verifypeer = true;

	// initialize SSL stuff
	// (OpenSSL >= 1.1.0 initializes itself automatically)
	//
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	SSL_library_init();
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
#endif
}

Socket_Comm_SSH::~Socket_Comm_SSH()
{
	this->finish();
}

void Socket_Comm_SSH::setHostname(const char *hostname)
{
	d_hostname = hostname ? hostname : "";
}

void Socket_Comm_SSH::setVerifyPeer(bool verify)
{
	d_verifypeer = verify;
}

void Socket_Comm_SSH::setSSLError(const char *prefix)
{
	string message = prefix ? prefix : "SSL error";

	if(ssl)
	{
		long verifyresult = SSL_get_verify_result(ssl);
		if(verifyresult != X509_V_OK)
		{
			message += ": certificate verification failed: ";
			message += X509_verify_cert_error_string(verifyresult);
			message += " (call setSSLVerify(false) to disable verification)";
		}
	}

	unsigned long code;
	char buffer[256];
	while((code = ERR_get_error()) != 0)
	{
		ERR_error_string_n(code, buffer, sizeof(buffer));
		message += ": ";
		message += buffer;
	}

	setError(message.c_str());
}

int Socket_Comm_SSH::virtualsend(const char *data, int length)
{
	if(!ssl)
	{
		setError("Socket_Comm_SSH virtualsend - SSL connection not established");
		return -1;
	}

	int timeoutsec = getTimeoutSecs();
	int timeoutmicrosec = getTimeoutMicroSecs();
	bool hastimeout = (timeoutsec > 0 || timeoutmicrosec > 0);

	// Same reasoning as virtualread(): a blocking socket lets SSL_write()
	// wedge in the kernel after select() said it was writable.
	//
	NonBlockingScope nonblocking(hastimeout ? getSocketDescriptor() : (SOCKET) -1);

	SocketClock::time_point deadline = SocketClock::now()
		+ std::chrono::seconds(timeoutsec)
		+ std::chrono::microseconds(timeoutmicrosec);

	bool wantread = false;

	while(1)
	{
		if(hastimeout)
		{
			struct timeval tv;
			if(!remainingUntil(deadline, &tv))
			{
				setError("Socket_Comm_SSH timed out on virtualsend");
				return -1;
			}

			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(getSocketDescriptor(), &fd);

			int maxDescriptor = (int) getSocketDescriptor() + 1;

			// A renegotiation can make SSL_write() need readability instead.
			//
			fd_set rfd;
			FD_ZERO(&rfd);
			if(wantread)
			{
				FD_SET(getSocketDescriptor(), &rfd);
			}

			int rc = select(maxDescriptor, wantread ? &rfd : (fd_set*) 0, &fd, (fd_set*) 0, &tv);
			if(rc < 0)
			{
				setError("Socket_Comm_SSH select failed for virtualsend");
				return -1;
			}
			if(rc == 0)
			{
				setError("Socket_Comm_SSH timed out on virtualsend");
				return -1;
			}
		}

		int rc = SSL_write(ssl, data, length);
		if(rc > 0)
		{
			return rc;
		}

		int sslerror = SSL_get_error(ssl, rc);
		if(sslerror == SSL_ERROR_WANT_READ || sslerror == SSL_ERROR_WANT_WRITE)
		{
			wantread = (sslerror == SSL_ERROR_WANT_READ);
			if(!hastimeout)
			{
				// Blocking socket: only reachable if the mode change failed.
				// Wait rather than spin.
				//
				fd_set fd;
				FD_ZERO(&fd);
				FD_SET(getSocketDescriptor(), &fd);
				int maxDescriptor = (int) getSocketDescriptor() + 1;
				if(select(maxDescriptor, (fd_set*) 0, &fd, (fd_set*) 0, (struct timeval*) 0) < 0)
				{
					setError("Socket_Comm_SSH select failed for virtualsend");
					return -1;
				}
			}
			continue;
		}
		wantread = false;
		if(sslerror == SSL_ERROR_ZERO_RETURN)
		{
			setError("Socket_Comm_SSH virtualsend - peer terminated");
			return -1;
		}
		setSSLError("Socket_Comm_SSH virtualsend - Send Failed");
		return -1;
	}
}

bool Socket_Comm_SSH::virtualfinish()
{
	if(ssl)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
		ssl = (SSL*) 0;
	}
	if(ctx)
	{
		SSL_CTX_free(ctx);
		ctx = (SSL_CTX*) 0;
	}
	return true;
}


bool Socket_Comm_SSH::virtualbind()
{
	// build the SSL objects...
	//
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	ctx = SSL_CTX_new(TLS_client_method());
#else
	ctx = SSL_CTX_new(SSLv23_client_method());
#endif

	if(!ctx)
	{
		setSSLError("Socket_Comm_SSH could not create SSL context");
		return false;
	}

	if(d_verifypeer)
	{
		// trust the system's default CA certificates
		//
		if(!SSL_CTX_set_default_verify_paths(ctx))
		{
			setSSLError("Socket_Comm_SSH could not load system CA certificates");
			return false;
		}
	}

	ssl = SSL_new(ctx);
	if(!ssl)
	{
		setSSLError("Socket_Comm_SSH could not create SSL structure");
		return false;
	}

	bool isip = d_hostname.empty() ? false : isIPLiteral(d_hostname.c_str());

	if(!d_hostname.empty() && !isip)
	{
		// SNI: modern servers require the hostname in the ClientHello,
		// or they respond with a handshake_failure alert.
		//
		SSL_set_tlsext_host_name(ssl, d_hostname.c_str());
	}

	if(d_verifypeer)
	{
		if(!d_hostname.empty() && !isip)
		{
			// require the certificate to match the hostname
			//
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
			SSL_set1_host(ssl, d_hostname.c_str());
#elif OPENSSL_VERSION_NUMBER >= 0x10002000L
			X509_VERIFY_PARAM_set1_host(SSL_get0_param(ssl), d_hostname.c_str(), 0);
#endif
		}
		SSL_set_verify(ssl, SSL_VERIFY_PEER, 0);
	}

	// assign the socket you created for SSL to use
	//
	SSL_set_fd(ssl, (int) getSocketDescriptor());

	// communicate!!
	/////////////////////////////////////////////
	int err = SSL_connect(ssl);
	if(err != 1)
	{
		setSSLError("Socket_Comm_SSH SSL handshake failed");
		return false;
	}
	//printf ("SSL connection using %s\n", SSL_get_cipher(ssl));
	return true;
}

int Socket_Comm_SSH::virtualread(char *buffer, int length)
{
	if(!ssl)
	{
		setError("Socket_Comm_SSH virtualread - SSL connection not established");
		return -1;
	}

	int timeoutsec = getTimeoutSecs();
	int timeoutmicrosec = getTimeoutMicroSecs();
	bool hastimeout = (timeoutsec > 0 || timeoutmicrosec > 0);

	// Non-blocking only while a timeout is in force, so the untimed case keeps
	// its original blocking behaviour.  If the mode change fails we still make
	// progress; we just cannot promise the deadline.
	//
	NonBlockingScope nonblocking(hastimeout ? getSocketDescriptor() : (SOCKET) -1);

	// One deadline for the whole call.  Re-arming the full timeout on every
	// iteration would let a peer that dribbles out records keep us here
	// indefinitely without ever sending application data.
	//
	SocketClock::time_point deadline = SocketClock::now()
		+ std::chrono::seconds(timeoutsec)
		+ std::chrono::microseconds(timeoutmicrosec);

	bool wantwrite = false;

	while(1)
	{
		// Only wait on the socket if OpenSSL has no decrypted data already
		// buffered: select() cannot see data sitting in OpenSSL's buffers,
		// so selecting while SSL_pending() > 0 could block forever.
		//
		if(SSL_pending(ssl) <= 0 && hastimeout)
		{
			struct timeval tv;
			if(!remainingUntil(deadline, &tv))
			{
				char msg[100];
				snprintf(msg, sizeof(msg), "Socket_Comm_SSH timed out on virtualread. Seconds: %d  Milli: %d", timeoutsec, timeoutmicrosec);
				setError(msg);
				return -1;
			}

			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(getSocketDescriptor(), &fd);

			int maxDescriptor = (int) getSocketDescriptor() + 1;

			// SSL_read() may need the socket to become writable instead (a
			// renegotiation mid-read), so wait on both.
			//
			fd_set wfd;
			FD_ZERO(&wfd);
			if(wantwrite)
			{
				FD_SET(getSocketDescriptor(), &wfd);
			}

			int rc = select(maxDescriptor, &fd, wantwrite ? &wfd : (fd_set*) 0, (fd_set*) 0, &tv);
			if(rc < 0)
			{
				setError("Socket_Comm_SSH select failed for virtualread");
				return -1;
			}
			if(rc == 0)
			{
				char msg[100];
				snprintf(msg, sizeof(msg), "Socket_Comm_SSH timed out on virtualread. Seconds: %d  Milli: %d", timeoutsec, timeoutmicrosec);
				setError(msg);
				return -1;
			}
		}

		int rc = SSL_read(ssl, buffer, length);
		if(rc > 0)
		{
			return rc;
		}

		int sslerror = SSL_get_error(ssl, rc);
		if(sslerror == SSL_ERROR_WANT_READ || sslerror == SSL_ERROR_WANT_WRITE)
		{
			// Incomplete TLS record, or a post-handshake record such as a
			// NewSessionTicket that carried no application data.  Wait for more.
			//
			wantwrite = (sslerror == SSL_ERROR_WANT_WRITE);
			if(!hastimeout)
			{
				// Blocking socket: SSL_read() only returns WANT_* here if the
				// mode change failed, so spinning would burn the CPU.  Wait for
				// readability with no deadline, matching the old behaviour.
				//
				fd_set fd;
				FD_ZERO(&fd);
				FD_SET(getSocketDescriptor(), &fd);
				int maxDescriptor = (int) getSocketDescriptor() + 1;
				if(select(maxDescriptor, &fd, (fd_set*) 0, (fd_set*) 0, (struct timeval*) 0) < 0)
				{
					setError("Socket_Comm_SSH select failed for virtualread");
					return -1;
				}
			}
			continue;
		}
		wantwrite = false;
		if(sslerror == SSL_ERROR_ZERO_RETURN)
		{
			setError("Socket_Comm_SSH virtualread - peer terminated");
			return 0;
		}
		if(sslerror == SSL_ERROR_SYSCALL && rc == 0)
		{
			// unclean EOF (peer closed without close_notify)
			//
			setError("Socket_Comm_SSH virtualread - peer terminated");
			return 0;
		}
		setSSLError("Socket_Comm_SSH virtualread - Receive Failed");
		return -1;
	}
}

}}

#endif



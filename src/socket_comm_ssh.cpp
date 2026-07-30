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

	while(1)
	{
		if(timeoutsec > 0 || timeoutmicrosec > 0)
		{
			struct timeval tv;
			tv.tv_sec = timeoutsec;
			tv.tv_usec = timeoutmicrosec;

			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(getSocketDescriptor(), &fd);

			int maxDescriptor = (int) getSocketDescriptor() + 1;

			int rc = select(maxDescriptor, (fd_set*) 0, &fd, (fd_set*) 0, &tv);
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
			// retry - select again first if a timeout is set
			//
			continue;
		}
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

	while(1)
	{
		// Only wait on the socket if OpenSSL has no decrypted data already
		// buffered: select() cannot see data sitting in OpenSSL's buffers,
		// so selecting while SSL_pending() > 0 could block forever.
		//
		if(SSL_pending(ssl) <= 0 && (timeoutsec > 0 || timeoutmicrosec > 0))
		{
			struct timeval tv;
			tv.tv_sec = timeoutsec;
			tv.tv_usec = timeoutmicrosec;

			fd_set fd;
			FD_ZERO(&fd);
			FD_SET(getSocketDescriptor(), &fd);

			int maxDescriptor = (int) getSocketDescriptor() + 1;

			int rc = select(maxDescriptor, &fd, (fd_set*) 0, (fd_set*) 0, &tv);
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
			// partial TLS record - wait for more data (select again if a timeout is set)
			//
			continue;
		}
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



// socket_connect_normal.cpp
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

#include "socket_connect_normal.h"


#ifndef INCLUDED_STDIO_H
#include <stdio.h>
#define INCLUDED_STDIO_H
#endif

#ifndef INCLUDED_STRING_H
#include <string.h>
#define INCLUDED_STRING_H
#endif

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

#ifndef INCLUDED_STDLIB_H
#include <stdlib.h>
#define INCLUDED_STDLIB_H
#endif

#ifndef INCLUDED_STRING
#include <string>
#define INCLUDED_STRING
#endif

#ifndef INCLUDED_ERRNO_H
#include <errno.h>
#define INCLUDED_ERRNO_H
#endif

#ifndef WIN32
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

using namespace std;

namespace rude
{
namespace sckt
{

//
// The two platforms report socket errors through different channels: errno on
// POSIX, WSAGetLastError() on Windows.  Everything below goes through these
// rather than touching errno directly, so the same logic can serve both.
//
static int socketError()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

static void setSocketError(int err)
{
#ifdef WIN32
	WSASetLastError(err);
#else
	errno = err;
#endif
}

//
// Puts the socket into non-blocking mode, or takes it back out.  Returns false
// if the mode could not be changed.
//
// Windows offers no way to read the current mode back, so the restore assumes
// the socket started out blocking.  Every socket this file works with was
// created here, so it did.
//
static bool setNonBlocking(SOCKET s, bool nonblocking)
{
#ifdef WIN32
	u_long mode = nonblocking ? 1 : 0;
	return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
	int flags = fcntl(s, F_GETFL);
	if(flags == -1)
	{
		return false;
	}
	int wanted = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
	return fcntl(s, F_SETFL, wanted) != -1;
#endif
}

//
// True when connect() has been accepted but not yet completed, which is what a
// non-blocking connect reports the moment it is issued.
//
static bool connectInProgress()
{
#ifdef WIN32
	return WSAGetLastError() == WSAEWOULDBLOCK;
#else
	return errno == EINPROGRESS || errno == EWOULDBLOCK;
#endif
}

//
// Renders the last socket error as text.
//
// Winsock does not report through errno -- ::connect() failures land in
// WSAGetLastError() -- so strerror(errno) on Windows described whatever
// unrelated CRT call touched errno last, if anything ever had.
//
static std::string lastSocketError()
{
#ifdef WIN32
	int err = WSAGetLastError();
	LPSTR text = NULL;
	DWORD len = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, (DWORD) err, 0, (LPSTR) &text, 0, NULL);
	std::string msg;
	if(len && text)
	{
		// FormatMessage leaves a trailing CRLF on most messages.
		while(len && (text[len - 1] == '\r' || text[len - 1] == '\n'))
		{
			--len;
		}
		msg.assign(text, len);
	}
	if(text)
	{
		LocalFree(text);
	}
	if(msg.empty())
	{
		char fallback[64];
		snprintf(fallback, sizeof(fallback), "Socket error %d", err);
		msg = fallback;
	}
	return msg;
#else
	return strerror(errno);
#endif
}

//
// Closes a descriptor this file opened, without disturbing the pending error.
//
static void closeSocket(SOCKET s)
{
	int saved = socketError();
#ifdef WIN32
	closesocket(s);
#else
	::close(s);
#endif
	setSocketError(saved);
}

//
// param: msec is the timeout in milliseconds
// returns: -1 on error; socketError() describes the underlying failure
//          -2 on timeout
//           0 if successful
//
// The error guarantee is the point of the SO_ERROR handling below.  A
// non-blocking connect() reports "in progress" immediately and the real
// outcome only lands in the socket's SO_ERROR, so a caller reading the error
// after this returned used to see "Operation now in progress" for every
// failure - a refused connection, an unreachable host, all of it.
//
// This whole function used to be a no-op on Windows: the non-blocking path sat
// inside #ifndef WIN32, leaving a plain blocking ::connect() that ignored msec
// entirely.  setTimeout() is documented to cover "Connect, Read and Send", and
// on Windows it covered two of the three -- a connect to an address that
// silently drops packets sat there for the stack's own SYN-retry period,
// roughly 21 seconds, no matter what the caller asked for.  Winsock does the
// same job through different names, so the difference is now confined to the
// helpers above.
//
int Socket_Connect_Normal::connecttimeout(int socket, struct sockaddr *addr, socklen_t len, int msec)
{
	int value; // Value to be returned

	// 1. Make sure the descriptor is non-blocking, so connect() returns
	//    immediately and select() gets to decide how long we wait.
	//
	if(!setNonBlocking(socket, true))
	{
		return -1;
	}

	// 2. Issue the connect request.
	//
	int ret = ::connect(socket, addr, len);

	if(ret == 0)
	{
		// Connected outright - a loopback peer that is already listening
		// often does.
		//
		value = 0;
	}
	else if(connectInProgress())
	{
		fd_set wset;
		fd_set eset;
		struct timeval tv;

		FD_ZERO(&wset);
		FD_SET(socket, &wset);

		// Windows signals a *failed* connect through the exception set
		// rather than the write set, so both have to be watched or a
		// refused connection would look like a timeout there.
		//
		FD_ZERO(&eset);
		FD_SET(socket, &eset);

		tv.tv_sec = msec / 1000;
		tv.tv_usec = (long) 1000 * (msec % 1000);

		ret = select(socket + 1, NULL, &wset, &eset, &tv);

		if(ret > 0 && (FD_ISSET(socket, &wset) || FD_ISSET(socket, &eset)))
		{
			int optval = 0;
			socklen_t optlen = sizeof(optval);

			// SO_ERROR is what separates "connected" from "failed" in
			// either set; the set it landed in only says it finished.
			//
			ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, (char *) &optval, &optlen);
			if(ret != 0)
			{
				// getsockopt() itself failed; its own error stands.
				value = -1;
			}
			else if(optval == 0)
			{
				value = 0;
			}
			else
			{
				// The connect failed.  optval holds why (ECONNREFUSED,
				// EHOSTUNREACH, ETIMEDOUT...); publish it so the caller's
				// error message names the actual problem.
				//
				setSocketError(optval);
				value = -1;
			}
		}
		else if(ret == 0)
		{
			value = -2; /* select timeout */
		}
		else
		{
			value = -1; /* select error */
		}
	}
	else
	{
		// Failed for a reason the stack knew about straight away.
		//
		value = -1;
	}

	// 3. Restore blocking mode.
	//
	// This can overwrite the pending error on success as well as on failure,
	// so save and restore it around the call; the diagnosis we just made is
	// more useful than anything this can report.
	//
	{
		int saved = socketError();
		if(!setNonBlocking(socket, false))
		{
			value = -1;
		}
		else
		{
			setSocketError(saved);
		}
	}

	return value;
}


Socket_Connect_Normal::~Socket_Connect_Normal()
{
	//	cout << __FILE__ << ": " << "~Socket_Connect_Normal()\n";
}


bool Socket_Connect_Normal::simpleConnect(SOCKET &sock, const char *server, int port)
{


	if(port <= 0)
	{
		setError("Port not set for HTTPConnect Object");
		return false;
	}
	if(server == (char *) NULL || strlen(server) == 0)
	{
		setError("Server Not Set for HTTPConnect Object");
		return false;
	}


	int len = strlen(server);
	char *buf = new char[len + 50];
	snprintf(buf, len + 50, "Normal connect to %s on port %d...", server, port);
	setMessage(buf);
	delete[] buf;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == (SOCKET) 0)
	{
		setError("Could not create socket");
		return false;
	}

	// Writing to a socket whose peer has gone raises SIGPIPE, and the
	// default disposition of SIGPIPE is to kill the process.  A peer that
	// disconnects at the wrong moment could therefore terminate any program
	// using this library, and there was nothing the program could do about
	// it from the outside short of changing a global signal handler - which
	// a library has no business requiring.
	//
	// SO_NOSIGPIPE turns that into a plain EPIPE for everything sent over
	// this descriptor, OpenSSL's writes included.  It is BSD and macOS only;
	// Linux has no equivalent socket option and uses MSG_NOSIGNAL per send
	// instead (see socket_comm_normal.cpp).
	//
#ifdef SO_NOSIGPIPE
	{
		int on = 1;
		setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (const char *) &on, sizeof(on));
	}
#endif



#ifdef WIN32

	// disable nagles algorithm
	//
	char on[1];
	on[0] = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, on, sizeof(on));

#endif


	struct sockaddr_in peer;
	memset(&peer, 0, sizeof(peer));
	peer.sin_family = AF_INET;
	peer.sin_port = htons(port);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *res = (struct addrinfo *) 0;
	if(getaddrinfo(server, (char *) 0, &hints, &res) != 0 || res == (struct addrinfo *) 0)
	{
		setError("Could not resolve domain");
		closeSocket(sock);
		sock = (SOCKET) 0;
		return false;
	}
	peer.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	freeaddrinfo(res);

	int milliseconds = getTimeoutSecs() * 1000;
	milliseconds += getTimeoutMicroSecs() / 1000;
	int rc;
	if(milliseconds)
	{
		rc = connecttimeout(sock, (struct sockaddr *) &peer, sizeof(peer), milliseconds);
	}
	else
	{
		rc = ::connect(sock, (struct sockaddr *) &peer, sizeof(peer));
	}
	if(rc)
	{
		if(rc == -2)
		{
			setError("Connect Timed Out");
		}
		else
		{
			setError(lastSocketError().c_str());
		}

		// This function opened the descriptor, so it owns it until it hands
		// back a connected one.  Leaving it open on failure leaked a
		// descriptor per attempt, which a caller that retries will notice
		// long before anything else goes wrong.
		//
		closeSocket(sock);
		sock = (SOCKET) 0;
		return false;
	}
	setMessage("Connected!!");
	return true;
}


} // namespace sckt
} // namespace rude

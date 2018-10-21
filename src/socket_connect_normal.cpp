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

namespace rude{
namespace sckt{

// 
// param: msec is the timeout in milliseconds 
// returns: -1 on error, errno is set 
//          -2 on timeout 
//           0 if successful 
// 
int Socket_Connect_Normal::connecttimeout(int socket, struct sockaddr *addr, socklen_t len, int msec) 
{ 
  int oldflags; // flags to be restored later
  int newflags; // nonblocking sockopt for socket 
  int ret;      // Result of syscalls 
  int value;    // Value to be returned 

#ifndef WIN32

  	// 1. ADJUST FLAGS
	//
	// store the current flags in oldflags
	// so we can restore them later
 	// oldflags = fcntl(socket, F_GETFL, 0);
	// F_GETFL option returns the current flags
	//
 	oldflags = fcntl(socket, F_GETFL); 
 	if(oldflags == -1)
 	{
		//cout << "Could not obtain old flags.....\n";
		//cout << strerror(errno);
   	return -1; 
	}

	// Make sure descriptor is non blocking...
	//
   newflags = oldflags | O_NONBLOCK; 

	// Update the socket with the new flags
	//
   if(fcntl(socket, F_SETFL, newflags) == -1)
	{
		//cout << "Could not update socket flags!!";
		//cout << strerror(errno);
		return -1; 
	}
#endif    

 
  // 2. CONNECT
  // Issue the connect request
  ret = ::connect(socket, addr, len); 

 
  if(ret == 0) 
  { 
    value = 0;
  }
#ifndef WIN32
  else if(errno == EINPROGRESS || errno == EWOULDBLOCK) 
  { 
		fd_set wset; 
		struct timeval tv; 
 
		FD_ZERO(&wset); 
		FD_SET(socket, &wset); 
 
		tv.tv_sec = 5; 
		// Why the hell is this times 1000?????
		//tv.tv_usec = 1000 * (msec % 1000); 
		tv.tv_usec = 0;// 1000 * msec % 1000;
 
		ret = select(socket+1, NULL, &wset, NULL, &tv); 
 
		if(ret == 1 && FD_ISSET(socket, &wset)) 
		{ 
			int optval; 
			socklen_t optlen = sizeof(optval); 
 
			ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval,&optlen); 
			if(ret == -1)
			{
				//cout << "Could not get socket options";
				//cout << strerror(errno);
				value= -1; 
 			}
			else if(optval==0)
			{
				value = 0; 
			}
			else 
			{
				//cout << "getSockOpt returned -1 in optval....";
				value = -1; 
			} 
		}
		else if(ret == 0) 
		{
			//cout << "Select timeout";
			//cout << strerror(errno);

			value = -2; /* select timeout */
		}
		else
		{
			//cout << "Select Error";
			//cout << strerror(errno);
		   value = -1; /* select error */ 
		}
   } 
#endif
	else
	{
		//cout << "Connect failed for good reason:";
		//cout << strerror(errno);
   	value = -1; 
	}
#ifndef WIN32

	//3. Restore the old flags
	//; 
   if(fcntl(socket, F_SETFL, oldflags) == -1)
	{
		//cout << "Error restoring Old flags";
		//cout << strerror(errno);
		value = -1; 
 	}
	
#endif

  return value; 
} 

Socket_Connect_Normal::~Socket_Connect_Normal()
{
	//	cout << __FILE__ << ": " << "~Socket_Connect_Normal()\n";
}

bool Socket_Connect_Normal::simpleConnect(SOCKET &sock, const char *server, int port)
{

	if(port <=0)
	{
		setError("Port not set for HTTPConnect Object");
		return false;
	}
	if(server == (char*) NULL || strlen(server) ==0)
	{
		setError("Server Not Set for HTTPConnect Object");
		return false;
	}

   int len=strlen(server);
	char *buf=new char[len + 50];
	sprintf(buf, "Normal connect to %s on port %d...", server, port);
	setMessage(buf);
	delete [] buf;

	sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock == (SOCKET) 0)
	{
		setError("Could not create socket");
		return false;
	}


	
#ifdef WIN32

	// disable nagles algorithm
	//
	char on[1];
	on[0]=1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, on, sizeof(on));
	
#endif


	struct sockaddr_in peer;
	LPHOSTENT he=gethostbyname(server);
	if(he==NULL)
	{
		setError("Could not resolve domain");
		return false;
	}
	peer.sin_family=AF_INET;
	peer.sin_port=htons(port);
	
	//peer.sin_addr.s_addr=inet_addr("12.34.167.1");

	peer.sin_addr=*(struct in_addr *)he->h_addr_list[0];

	int milliseconds = getTimeoutSecs() * 1000;
	milliseconds += getTimeoutMicroSecs();
	int rc;
	if(milliseconds)
	{
		rc= connecttimeout(sock, (struct sockaddr *)&peer, sizeof(peer), milliseconds);
	}
	else
	{
		rc= ::connect(sock, (struct sockaddr *)&peer, sizeof(peer));
	}
	if(rc)
	{
		if(rc == -1)
		{
			setError(strerror(errno));
		}
		if(rc == -2)
		{
			setError("Connect Timed Out");
		}
		return false;
	}
	setMessage("Connected!!");
	return true;
}

}}


// socket_comm.cpp
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


#include "socket_comm.h"

#ifndef INCLUDED_STRING_H
#include <string.h>
#define INCLUDED_STRING_H
#endif

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

using namespace std;


namespace rude{
namespace sckt{

Socket_Comm::Socket_Comm()
{
	d_sock=(SOCKET) 0;
	d_error = "";
	d_timeoutsecs=0;
	d_timeoutmicrosecs=0;
}

Socket_Comm::~Socket_Comm()
{
	// children should call finish() in their destructors!!!!!
	// we can't call it here since children's destructors are called first...
	//
}

void Socket_Comm::setError(const char *error)
{
	d_error=error;
}

const char *Socket_Comm::getError() const
{
	return d_error.c_str();
}

void Socket_Comm::setTimeout(int secs, int microsecs)
{
	d_timeoutsecs=secs;
	d_timeoutmicrosecs=microsecs;
}

int Socket_Comm::getTimeoutSecs() const
{
	return d_timeoutsecs;
}

int Socket_Comm::getTimeoutMicroSecs() const
{
	return d_timeoutmicrosecs;;
}

bool Socket_Comm::bind(SOCKET &sock)
{
	// this binds our object to the physical integer holding the file descriptor....
	//
	if(d_sock != (SOCKET) 0)
	{
		// if d_sock already points to a socket, it is already bound....
		//
		setError("Socket_Comm::Bind - already bound (call finish first)");
		return false;
	}
	else if(sock == (SOCKET) 0)
	{
		// if the socket passed in isn't a valid file descriptor, then we can't bind to it...
		//
		setError("Socket_Comm::Bind - Socket Descriptor is not valid");
		return false;
	}
	else
	{
		d_sock=sock;
		if(virtualbind())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

SOCKET Socket_Comm::getSocketDescriptor()
{
	return d_sock;
}
	
bool Socket_Comm::finish()
{
	if(d_sock != (SOCKET) 0)
	{
		d_sock = (SOCKET) 0;
		return virtualfinish();

	}
	setError("Socket_Comm::finish - already finished");
	return false;
}

// normal socket send with timeout
//
int Socket_Comm::send(const char *data, int length)
{
	if(d_sock != (SOCKET) 0)
	{
		return virtualsend(data, length);
	}
	else
	{
		setError("Socket_Comm::send - Not bound to any socket Yet!!");
		return -1;
	}
}

// normal recv with timeout
//
int Socket_Comm::read(char *buffer, int length)
{
	if(d_sock != (SOCKET) 0)
	{
		return virtualread(buffer, length);
	}
	else
	{
		setError( "Socket_Comm::read - Not bound to any socket Yet!!!!");
		return -1;
	}
}


}}


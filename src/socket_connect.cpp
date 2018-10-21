// socket_connect.cpp
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


#include "socket_connect.h"

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

Socket_Connect::Socket_Connect()
{
	d_error = "";
	d_server = "";
	d_port = 0;
	d_timeoutsecs = 0;
	d_timeoutmicrosecs = 0;
	s_nextchild = 0;
}
	
Socket_Connect::~Socket_Connect()
{
	delete s_nextchild;
}

void Socket_Connect::setServer(const char *server)
{
	if(server != (char*) NULL)
	{
		d_server=server;
	}
	else
	{
		d_server="";
	}
}

void Socket_Connect::setPort(int port)
{
	d_port=port;
}

void Socket_Connect::setTimeout(int seconds, int microseconds)
{
	d_timeoutsecs = seconds;
	d_timeoutmicrosecs = microseconds;
	if(s_nextchild)
	{
		s_nextchild->setTimeout(seconds, microseconds);
	}
}
	
int Socket_Connect::getTimeoutSecs() const
{
	return d_timeoutsecs;
}
	
int Socket_Connect::getTimeoutMicroSecs() const
{
	return d_timeoutmicrosecs;;
}

const char *Socket_Connect::getServer() const
{
	return d_server.c_str();
}
	
int Socket_Connect::getPort() const
{
	return d_port;
}

bool Socket_Connect::connect(SOCKET &s, const char *destserver, int destport)
{
	// if we have a child server, then we need to connect
	// to the child's address, otherwise, we connect to whatever
	// we were asked to connect to
	if(s_nextchild != NULL)
	{
		if(simpleConnect(s, s_nextchild->getServer(), s_nextchild->getPort()))
		{
			if(s_nextchild->connect(s, destserver, destport))
			{
				return true;
			}
			setError(s_nextchild->getError());
			return false;
		}
		return false;
	}
	else
	{
		return simpleConnect(s, destserver, destport);
	}
}

	
void Socket_Connect::setMessage(const char *message)
{
	// Currently for debug only-
	// will have client specify "verbose" to
	// make this method happen in the future
	//
	//cout << ":::" << message <<"\n";
}

void Socket_Connect::setError(const char *error)
{
	d_error = error ? error : "";
}

Socket_Connect *Socket_Connect::getChild() const
{
	return s_nextchild;
}

void Socket_Connect::setChild(Socket_Connect *nextchild)
{
	if(s_nextchild)
	{
		delete s_nextchild;
	}
	s_nextchild = nextchild;
}
	
const char *Socket_Connect::getError() const
{
		return d_error.c_str();
}

}}
	

// socket_tcpclient_normal.cpp
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


#include "socket_tcpclient.h"

#ifndef INCLUDED_SOCKET_CONNECT_H
#include "socket_connect.h"
#endif

#ifndef INCLUDED_SOCKET_COMM_H
#include "socket_comm.h"
#endif

#ifndef INCLUDED_SOCKET_PLATFORM_H
#include "socket_platform.h"
#endif

#ifndef INCLUDED_STRING_H
#include <string.h>
#define INCLUDED_STRING_H
#endif

#ifndef INCLUDED_STDIO_H
#include <stdio.h>
#define INCLUDED_STDIO_H
#endif

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

using namespace std;

namespace rude{
namespace sckt{

Socket_TCPClient::Socket_TCPClient()
{
	timeoutsecs = 0;
	timeoutmilli = 0;
	d_connection= 0;
	d_comm= 0;
	d_error="";
	d_sock= 0;
}

Socket_TCPClient::~Socket_TCPClient()
{
	if(d_connection)
	{
		delete d_connection;
	}
	if(d_comm)
	{
		delete d_comm;
	}
}

void Socket_TCPClient::setError(const char *error)
{
	d_error= error? error : "";
}

void Socket_TCPClient::setTimeout(int seconds, int microseconds)
{
	timeoutsecs = seconds;
	timeoutmilli=microseconds;
	
	if(d_connection != (Socket_Connect*) 0)
	{
		d_connection->setTimeout(seconds, microseconds);
	}
	
	if(d_comm != (Socket_Comm*) 0)
	{
		d_comm->setTimeout(seconds, microseconds);;
	}
}

	
void Socket_TCPClient::setConnection(Socket_Connect *connection)
{
	if(d_connection != (Socket_Connect*) 0)
	{
		delete d_connection;
	}
	d_connection=connection;
	d_connection->setTimeout(timeoutsecs, timeoutmilli);
}

void Socket_TCPClient::setComm(Socket_Comm *comm)
{
	if(d_comm != (Socket_Comm*) 0)
	{
		delete d_comm;
	}
	d_comm=comm;
	d_comm->setTimeout(timeoutsecs, timeoutmilli);
}

const char *Socket_TCPClient::getError()
{
		return d_error.c_str();
}

int Socket_TCPClient::send(const char *data, int length)
{
	if(d_comm != (Socket_Comm*) 0)
	{
		int ret=d_comm->send(data, length);
		if(ret <= 0)
		{
			setError(d_comm->getError());
		}
		return ret;
	}
	else
	{
		setError("Socket_TCPClient does not have a Socket_Comm to send through");
		return -1;
	}
}
	
int Socket_TCPClient::read(char *buffer, int length)
{
	if(d_comm != (Socket_Comm*) 0)
	{
		int ret=d_comm->read(buffer, length);
		if(ret <= 0)
		{
			setError(d_comm->getError());
		}
		return ret;
	}
	else
	{
		setError("Socket_TCPClient does not have a Socket_Comm to read through");
		return -1;
	}
}

// reads until peer terminates, timeout, or NULL character read
//
const char *Socket_TCPClient::reads()
{
	d_readbuffer="";
	char c;
	int rc;
	while(1)
	{
		rc=this->read(&c, 1);
		if(rc > 0)
			
		{
			d_readbuffer += c;
			if(c == (char) 0)
			{
				return d_readbuffer.c_str();
			}	
		}
		else if(rc == 0)
		{
			// peer terminated
			//
			return d_readbuffer.c_str();
		}
		else
		{
			// read error or timeout error
           d_readbuffer="";
			return (char*) 0;
		}
	}
}
	
// reads until crlf encountered. discards crlf...
//
const char *Socket_TCPClient::readline()
{
	d_readbuffer="";
	char c;
	char lastc=(char) 0; 
	int rc;
	while(1)
	{
		rc=this->read(&c, 1);
		if(rc > 0)
		{
			if(c != 0x0a && c != 0x0d)
			{
				d_readbuffer += c;
			}
			if(c == (char) 0)
			{
				return d_readbuffer.c_str();
			}
			if(c == 0x0a && lastc == 0x0d)
			{
				return d_readbuffer.c_str();
			}
			lastc=c;
		}
		else if(rc == 0)
		{
			// peer terminated
			//
			return d_readbuffer.c_str();
		}
		else
		{
			// read error or timeout error
			d_readbuffer="";
			return (char*) NULL;
		}
	}
}

bool Socket_TCPClient::sends(const char *buffer)
{
	if(buffer == (char*) 0)
	{
		setError("Socket_TCPClient cant sends null pointer");
		return false;
	}
	int len=strlen(buffer);

	// TRICKY!!!
	//
	if(len==0)
	{
		return true;
	}

	if(d_comm != (Socket_Comm*) 0)
	{
		int ret=d_comm->send(buffer, len);
		if(ret <= 0)
		{
			setError(d_comm->getError());
			return false;
		}
		return true;
	}
	else
	{
		setError("Socket_TCPClient does not have a Socket_Comm to sends through");
		return false;
	}
}
	
bool Socket_TCPClient::connect(const char *server, int port)
{
	if(d_connection != (Socket_Connect*) 0)
	{
		if(d_connection->connect(d_sock, server, port))
		{
			if(d_comm->bind(d_sock))
			{
				return true;
			}
			else
			{
				setError(d_comm->getError());
				return false;
			}
		}
		else
		{
			setError(d_connection->getError());
			return false;
		}
	}
	else
	{
		setError("Socket_TCPClient::connect does not have a Socket_Connect to connect through");
		return false;
	}
}
	
bool Socket_TCPClient::close()
{
	if(d_comm != (Socket_Comm*) 0)
	{
		if(d_comm->finish())
		{
		
#ifdef WIN32
			closesocket(d_sock);
#else
			::shutdown(d_sock, SHUT_RDWR);
			::close(d_sock);
#endif
			return true;
		}
		else
		{
			setError(d_comm->getError());
			return false;
		}
	}
	else
	{
		setError("Socket_TCPClient::close does not have a Socket_Comm to close");
		return false;
	}
}
}}


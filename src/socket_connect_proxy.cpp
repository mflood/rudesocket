// socket_connect_proxy.cpp
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

#include "socket_connect_proxy.h"

#ifndef INCLUDED_SOCKET_COMM_NORMAL_H
#include "socket_comm_normal.h"
#endif

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

using namespace std;

namespace rude{
namespace sckt{

Socket_Connect_Proxy::Socket_Connect_Proxy()
{
	s_socketcomm=new Socket_Comm_Normal();
}
	
Socket_Connect_Proxy::~Socket_Connect_Proxy()
{
	delete s_socketcomm;
}

bool Socket_Connect_Proxy::simpleConnect(SOCKET &s, const char *server, int port)
{
	// we're using a socket_comm to do the talking...
	//
	s_socketcomm->bind(s);
	s_socketcomm->setTimeout( getTimeoutSecs() ,getTimeoutMicroSecs());
	
	// we are expecting a socket that is already connected...
	// and we expect destip and destport to be valid....
	if(port <=0)
	{
		setError("Port not set for HTTP Proxy connection");
		return false;
	}
	if(server == (char*) NULL || strlen(server) ==0)
	{
		setError("Server Not Set for HTTP Proxy connection");
		return false;
	}
	if(s == (SOCKET) 0)
	{
		setError("Socket is not connected yet! Can't initiate HTTP Proxy connection");
		return false;
	}

	int len=strlen(server);
	char *temp = new char[len + 100];		
	sprintf(temp,  "HTTP Proxy opening %s on port %d...", server, port);
	setMessage(temp);
	delete [] temp;
		
	// everything is ok, do the connect thingy.....
	// tell the proxy server where to connect to
	//
	char *buf=new char[200];
		
	sprintf(buf, "CONNECT %s:%d HTTP/1.1\n\n",server, port);
		
	int rc=s_socketcomm->send( buf, strlen(buf));
		
     if(rc <=0)
     {
     	setError("Socket_Connect_Proxy::simpleConnect() failed. Could not send()");
		delete [] buf;
     	return false;
     }

	// NOW WE Get back a normal HTTP response ... a 2xx response code means we were successful
	// eg. "HTTP/1.0 200 Connection Established" = good
	// eg. "HTTP/1.0 404 Not Found" = bad

	// so read in 12 bytes of data
		
	rc=s_socketcomm->read( buf, 12);

	if(rc < 0)
	{
		setError(s_socketcomm->getError());
		delete [] buf;
		return false;
	}
	if(rc == 0)
	{
		setError(s_socketcomm->getError());
		delete [] buf;
		return false;
	}
	
	// not really needed since we're not printing it...
	//
	buf[rc]=(char) NULL;
	if(rc < 12)
	{
		delete [] buf;
		setError("HTTP Proxy Connection failed - did not send back enough info");
		return false;		
	}
		
	// check that position 9 of return buf is a 2

	if(buf[9] != '2')
	{
		int returncode;
		sscanf(&buf[9], "%d", &returncode);
	
		delete [] buf;
		char *temp = new char[100];
		sprintf(temp, "HTTP Proxy connection failed (%d)", returncode);
		
		setError(temp);
		delete [] temp;
		
		return false;
	}

	delete [] buf;

	// now we need to get down to the end of the Proxy Headers to enable conversation
	// with the destination server....
	// we're looking for 0x0D 0x0A 0x0D 0x0A (CRLF CRLF)
	char c;
	char lastchar=(char)0;
	char seclastchar=(char) 0;
	while(1)
	{
		rc=s_socketcomm->read(  &c, 1);
		//cout << c;
		if(rc > 0)
		{
			if(c == 0x0a)
			{
				if(seclastchar == c)
				{
					setMessage("HTTP Proxy Connected!!\n");
					return true;
				}
			}
			seclastchar=lastchar;
			lastchar = c;
		}
		else
		{
			break;
		}
	}
	setError("Proxy Connection was good, but could not read entire set of headers...");
	return false;
}

}}

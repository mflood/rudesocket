// socket_connect_socks4.cpp
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

#include "socket_connect_socks4.h"
#include "socket_comm_normal.h"

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

Socket_Connect_Socks4::Socket_Connect_Socks4()
{
	d_username= "";
	d_buffer[0]=(char) 0;
	s_socketcomm=new Socket_Comm_Normal();
}
	
Socket_Connect_Socks4::~Socket_Connect_Socks4()
{
	delete s_socketcomm;
}

void Socket_Connect_Socks4::setUsername(const char *username)
{
	d_username = username ? username : "";
}

bool Socket_Connect_Socks4::simpleConnect(SOCKET &s, const char *server, int port)
{
	setMessage("Socks 4 establishing connection");
	
	if(port <=0)
	{
		setError("Port not set for Socket_Connect_Socks4 Object");
		return false;
	}
	if(server == (char*) NULL || strlen(server) ==0)
	{
		setError("Server Not Set for Socket_Connect_Socks4 Object");
		return false;
	}

	// we are expecting a socket that is already connected...
	// and we expect destip and destport to be valid....
		if(s == (SOCKET) 0)
	{
		setError("Socket is not connected yet! for Socket_Connect_Socks4 object");
		return false;
	}

	// configure communication object
	//
	s_socketcomm->bind(s);
	s_socketcomm->setTimeout( getTimeoutSecs() ,getTimeoutMicroSecs());

	setMessage("Socks4 requesting CONNECT operation");
	LPHOSTENT he=gethostbyname(server);
		
	if(he==NULL)
	{
		setError("Could not resolve domain");
		return false;
	}
	const char *ipaddress = he->h_addr_list[0];
		
	// First 2 bytes are version and command
	//
	d_buffer[0]=0x04;	// version 4
	d_buffer[1]=0x01;	// Command code is 1 (connect)
	
	// next 2 bytes are for the port
	//
	d_buffer[2]=0; //(char) (port / 256); // high bits of port
	d_buffer[3]=0x50; // (char) (port % 256); // low bits of port							

	// next 4 bytes are for the ip address
	//
	d_buffer[4]=ipaddress[0];	
	d_buffer[5]=ipaddress[1];	
	d_buffer[6]=ipaddress[2];	
	d_buffer[7]=ipaddress[3];

	d_buffer[8]=0;

	//next bytes are for user id - 
	// this is variable NULL terminated length
	//int len=strlen(d_username);
	//for(int x=0; x< len; x++)
	//{
	//	d_buffer[8 + x] = d_username[x];
	//}
	//d_buffer[8 + len]=0;

	int rc=s_socketcomm->send(d_buffer, 9);
     if(rc <=0)
     {
     	setError("Socket_Connect_Socks4::simpleConnect() failed. Could not send version/method");
     	return false;
     }
	  
	setMessage("Socks 4 reading response\n");
	
	// STEP 2. The server selects from one of the methods given in METHODS, and
  	// sends a METHOD selection message (2 octets):
	//
	rc=s_socketcomm->read(d_buffer, 8);
		
	if(rc < 8)
	{
		// error ....
		char *temp = new char[1000];
		sprintf(temp, "Socks4 only sent back %d bytes, expecting 8. %s", rc, s_socketcomm->getError());
		
		setError(temp);
		delete [] temp;
		return false;
	}

	// check version 
	//
	if(d_buffer[0] != 0)
	{
		setError("Socks Server returned bad version");
		return false;
	}
		
	switch(d_buffer[1])
	{
		case 90:
			setMessage("Socks4 - Request Granted. Connected!!");
			return true;
		case 91:
			setError("Socks4 - Request Rejected");
                               return false;
		case 92:
			setError("Socks4 - Could not connect to destination");
                               return false;
		case 93:
			setError("Socks4 - Invalid UserID");
                               return false;
		default:
			setError("Socks4 sent back Invalid Reponse Code - Unknown Error");
                               return false;
	}
}
}}


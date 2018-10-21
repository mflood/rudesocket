// socket_connect_socks5.cpp
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


#include "socket_connect_socks5.h"

#ifndef INCLUDED_STDIO_H
#include <stdio.h>
#define INCLUDED_STDIO_H
#endif

#ifndef INCLUDED_STRING_H
#include <string.h>
#define INCLUDED_STRING_H
#endif

namespace rude{
namespace sckt{

Socket_Connect_Socks5::Socket_Connect_Socks5()
{
	d_username="";
	d_password="";
	d_buffer[0]=(char) 0;
}
	
Socket_Connect_Socks5::~Socket_Connect_Socks5()
{

}

void Socket_Connect_Socks5::setUsername(const char *username)
{
	d_username = username ? username: "";
}

void Socket_Connect_Socks5::setPassword(const char *password)
{
	d_password = password ? password : "";
}

bool Socket_Connect_Socks5::simpleConnect(SOCKET &s, const char *server, int port)
{

	setMessage("Socks 5 establishing connection");
	// The port and server must be specified before we can connect
	//
	if(port <=0)
	{
		setError("Port not set for Socket_Connect_Socks5 Object");
		return false;
	}

	if(server == (char*) NULL || strlen(server) ==0)
	{
		setError("Server Not Set for Socket_Connect_Socks5 Object");
		return false;
	}

	
	// we are expecting a socket that is already connected...
	// and we expect destip and destport to be valid....
	if(s == (SOCKET) 0)
	{
		setError("Socket is not connected yet! for Socket_Connect_Socks5 object");
		return false;
	}

	// configure communication object
	//
	d_socketcomm.bind(s);
	d_socketcomm.setTimeout( getTimeoutSecs() ,getTimeoutMicroSecs());

	// Negotiate with Socks Server
	// see RFC 1928
	//
	
	setMessage("Socks 5 sending version method\n");
	
	// STEP 1. send version ID and Method selections
	//
	d_buffer[0]=0x05;	// version 5
	d_buffer[1]=0x02;	// we're including two methods
	d_buffer[2]=0x00;	// no auth required method
	d_buffer[3]=0x02;	// username / password method
	
	// ALLOWED METHODS
     //    o  X'00' NO AUTHENTICATION REQUIRED
     //    o  X'01' GSSAPI (RFC 1961)
     //    o  X'02' USERNAME/PASSWORD (RFC 1929)
     //    o  X'03' to X'7F' IANA ASSIGNED
     //    o  X'80' to X'FE' RESERVED FOR PRIVATE METHODS
     //	   o  X'FF' NO ACCEPTABLE METHODS
	
	
	int rc=d_socketcomm.send(d_buffer, 4);
	if(rc <=0)
   {
    	setError("Socket_Connect_Socks5::simpleConnect() failed. Could not send version/method");
     	return false;
   }

	
	setMessage("Socks5 reading response");
	
	// STEP 2. The server selects from one of the methods given in METHODS, and
  	// sends a METHOD selection message (2 octets):
	//
	rc=d_socketcomm.read(d_buffer, 2);
		
		if(rc < 2)
		{
			// error ....
			return false;
		}

		// check version 
		//
		if(d_buffer[0] != 0x05)
		{
			setError("Socks Server returned bad version");
			return false;
		}
		
		// check method
		//
		if(d_buffer[1] == (char)0xff)
		{
			setError("Socks Server rejected our suggested methods");
			return false;
		}

		// check for username/password
		//
		if(d_buffer[1] == 0x02)
		{
			// perform username / password authentication
			// described in RFC 1929
			//
			int x=0;
			int y;
			int ulen=d_username.size();
			int plen=d_password.size();	
			d_buffer[x++]=0x01;	// version 1
			d_buffer[x++]=(char) ulen;
			for(y=0; y< ulen; y++)
			{
				d_buffer[x++]=d_username.c_str()[y];
			}
			d_buffer[x++]=(char) plen;
			for(y=0; y< plen; y++)
			{
				d_buffer[x++]=d_password.c_str()[y];
			}

			setMessage("Socks5 sending username/ password authentication");
			
			int rc=d_socketcomm.send(d_buffer, x);
	      if(rc <=0)
	      {
	      	setError("Socket_Connect_Socks5::simpleConnect() failed. Could not send username/password");
	      	return false;
	      }
	
			// STEP 2. The server returns 0 if username/password are correct
	   	// (2 octets):
			//
			setMessage("Socks5 reading authentication response");

			rc=d_socketcomm.read(d_buffer, 2);
			// printf("return status: %d\n", rc);
			if(rc < 2)
			{
			    char status[80];
				sprintf(status, "Could not read authentication response, received %d bytes instead of 2", rc);
				setError(status);
				return false;
			}

			// check version 
			//
			if(d_buffer[1] != 0x00)
			{
				setError("Socks Server rejected username / password");
				return false;
			}
		}

	// STEP 3
	// we've been authenticated, now send connection instructions...

	setMessage("Socks5 sending connection instructions");

	int x=0;
	int y;
	int servlen=strlen(server);
	d_buffer[x++]=0x05;	// version 5
	d_buffer[x++]=0x01;	// Connect
	d_buffer[x++]=0x00;	// reserved
	d_buffer[x++]=0x03;	// address type

	//       o  IP V4 address: X'01'
	//       o  DOMAINNAME: X'03'
	//       o  IP V6 address: X'04'
	d_buffer[x++]=servlen;
	for(y=0; y< servlen; y++)
	{
		d_buffer[x++]=server[y];
	}
	d_buffer[x++]=(char) (port / 256); // high bits of port
	d_buffer[x++]=(char) (port % 256); // low bits of port

	rc=d_socketcomm.send( d_buffer, x);
	if(rc <=0)
	{
		setError("Socket_Connect_Socks5::simpleConnect() failed. Could not send connection instructions");
		return false;
	}

	// STEP 4. The server returns evaluation results
  	// 
	//
	//       o  X'00' succeeded
	//       o  X'01' general SOCKS server failure
	//       o  X'02' connection not allowed by ruleset
	//       o  X'03' Network unreachable
	//       o  X'04' Host unreachable
	//       o  X'05' Connection refused
	//       o  X'06' TTL expired
	//       o  X'07' Command not supported
	//       o  X'08' Address type not supported
	//       o  X'09' to X'FF' unassigned

	rc=d_socketcomm.read( d_buffer, 2);

	if(rc < 2)
	{
		setError(d_socketcomm.getError());
		// error ....
		//setError("Could not read Socks Cap servers evaluation response 1 (timed out?)\n");
		return false;
	}

	char reponsecode=d_buffer[1];
					 
	rc=d_socketcomm.read( d_buffer, 2);
	if(rc < 2)
	{
		// error ....
		setError("Could not read Socks Cap servers evaluation response 2 (timed out?)\n");
		return false;
	}

	// now we need to read in the rest of the server's response data
	// X'00' |  1   | Variable |    2
	// o  IP V4 address: X'01'
	// o  DOMAINNAME: X'03'
	// o  IP V6 address: X'04'

	if(d_buffer[1] == 0x01)
	{
		d_socketcomm.read( d_buffer, 6);
	}
	else if(d_buffer[1] == 0x03)
	{
		d_socketcomm.read( d_buffer, 1);
		d_socketcomm.read(d_buffer, (d_buffer[0] + 2));
	}
	else if(d_buffer[1] == 0x04)
	{
		// read in 18
		d_socketcomm.read( d_buffer, 18);
	}

	switch(reponsecode)
	{
		case 0x00:
			setMessage("Socks 5 Connected!");
			return true;
		case 0x01:
			setError("general SOCKS server failure");
			return false;
		case 0x02:
			setError("connection not allowed by ruleset");
			return false;
		case 0x03:
			setError("Network unreachable");
			return false;
		case 0x04:
			setError("Host unreachable");
			return false;
		case 0x05:
			setError("Connection refused");
			return false;
		case 0x06:
			setError("TTL expired");
			return false;
		case 0x07:
			setError("Command not supported");
			return false;
		case 0x08:
			setError("Address type not supported");
			return false;
		default:
			setError("Unknown Error");
			return false;
	}
}

}}

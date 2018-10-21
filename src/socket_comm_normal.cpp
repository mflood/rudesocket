// socket_comm_normal.cpp
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


#include "socket_comm_normal.h"

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

#ifndef INCLUDED_STDIO_H
#include <stdio.h>
#define INCLUDED_STDIO_H
#endif

using namespace std;

namespace rude{
namespace sckt{

Socket_Comm_Normal::~Socket_Comm_Normal()
{
	this->finish();
}
	
int Socket_Comm_Normal::virtualsend(const char *data, int length)
{
	int rc;
	int x;

	int timeoutsec=getTimeoutSecs();
	int timeoutmicrosec=getTimeoutMicroSecs();

	if(timeoutsec > 0 || timeoutmicrosec > 0 )
	{
		for(x=0; x< length; x++)
		{
			// prepare timeout struct...
			//
			struct timeval tv;
			tv.tv_sec=timeoutsec;
			tv.tv_usec=timeoutmicrosec;

			// set up file decriptorset
			//
			fd_set fd;
			
			// clear it each time
			//
			FD_ZERO(&fd);

			// add socket to file descriptor set
			//
			FD_SET(getSocketDescriptor(), &fd);

			//determine maximum file descriptor number for select to check
			// since we're only using one file descriptor (the socket) then we already
			// know what it is....
			
			int maxDescriptor = getSocketDescriptor() + 1;
			
			// do the select thingy
			//
			rc=select(maxDescriptor, (fd_set*) 0, &fd, (fd_set*) 0, &tv);
			
			if(rc < 0)
			{
				setError("Socket_Comm_Normal select failed for virtualsend");
				return -1;
			}
			
			if(rc == 0)
			{
				setError("Socket_Comm_Normal timed out on virtualsend");
				// timed out
				return x;
			}
			
			if(!FD_ISSET(getSocketDescriptor(), &fd))
			{
				setError("Socket_Comm_Normal virtualsend - select returned invalid FD");
				return -1;
			}

 			// perform read function
			//      
			rc = ::send(getSocketDescriptor(), (char *) &data[x], 1,0);
		
			if(rc==0)
			{
				setError("Socket_Comm_Normal virtualsend - peer terminated");
				return -1;
			}
			if(rc < 0)
			{
				setError("Socket_Comm_Normal virtualsend - Send Failed");
				return -1;
			}
		}
	}
	else
	{
		x = ::send(getSocketDescriptor(), data, length, 0);

		if(x==0)
		{
			setError("Socket_Comm_Normal virtualsend - peer terminated");
			return x;
		}
		if(x < 0)
		{
			setError("Socket_Comm_Normal virtualsend - Send Failed");
			return -1;
		}
	}
	
	return x;
}


bool Socket_Comm_Normal::virtualfinish()
{
	// Nothing to do for normal communication
	
	return true;
}


bool Socket_Comm_Normal::virtualbind()
{
	// Nothing to do for normal communication
	
	return true;
}



int Socket_Comm_Normal::virtualread(char *buffer, int bufferlength)
{
	int rc; // return code
	

	int timeoutsec=getTimeoutSecs();
	int timeoutmicrosec=getTimeoutMicroSecs();

	int x;
	for(x=0; x< bufferlength; x++)
	{
		if(timeoutsec > 0 || timeoutmicrosec > 0 )
		{
			// prepare timeout struct...
			//
			struct timeval tv;
			tv.tv_sec=timeoutsec;
			tv.tv_usec=timeoutmicrosec;

			// set up file decriptorset
			//
			fd_set fd;
			
			// clear the fiel descriptor each time
			//
			FD_ZERO(&fd);

			// add socket to file descriptor set
			//
			FD_SET(getSocketDescriptor(), &fd);

			// determine maximum file descriptor number for select to check
			// since we're only using one file descriptor (the socket) then we already
			// know what it is....
			
			int maxDescriptor = getSocketDescriptor() + 1;
			
			// do the select thingy
			//
			rc=select(maxDescriptor, &fd, (fd_set*) 0, (fd_set*) 0, &tv);
			
			if(rc < 0)
			{
				setError("Socket_Comm_Normal select failed for virtualread");
				return -1;
			}
			
			if(rc == 0)
			{
				char *buffer = new char[100];
				sprintf(buffer, "Socket_Comm_Normal timed out on virtualread. Seconds: %d  Milli: %d", timeoutsec, timeoutmicrosec);
				setError(buffer);
				delete [] buffer;
				// timed out
				return -1;
			}
			
			if(!FD_ISSET(getSocketDescriptor(), &fd))
			{
				setError("Socket_Comm_Normal virtualread - select returned invalid FD");
				return -1;
			}
		}

 		// perform read function
		//      
		rc=recv(getSocketDescriptor(), (char *) &buffer[x], 1,0);
		
		if(rc==0)
		{
			setError("Socket_Comm_Normal virtualread - peer terminated");
			return 0;
		}
		if(rc < 0)
		{
			setError("Socket_Comm_Normal virtualread - Receive Failed");
			return -1;
		}
	}
	
	return x;
}

}}


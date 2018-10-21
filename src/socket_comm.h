// socket_comm.h
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


#ifndef INCLUDED_SOCKET_COMM_H
#define INCLUDED_SOCKET_COMM_H

#ifndef INCLUDED_SOCKET_PLATFORM_H
#include "socket_platform.h"
#endif

#ifndef INCLUDED_STRING
#include <string>
#define INCLUDED_STRING
#endif

namespace rude{
namespace sckt{

//=
// Defines an interface for communicating over an already connected socket.
// 
// TODO: make d_sock a reference instead of an integer.......
//
//=
class Socket_Comm{

	std::string d_error;
	int d_timeoutsecs;
	int d_timeoutmicrosecs;
	SOCKET d_sock;
	
protected:
	
	Socket_Comm();
	void setError(const char *error);

	//=
	// Subclasses implement to perform the real send
	// Should return number of bytes sent, or -1 for error.
	// If an error occurs, subclasses should set the appropriate errno/errstring
	//=
	virtual int virtualsend(const char *buffer, int length)=0;
	
	//=
	// Subclasses implement to perform the real read
	// Should return number of bytes received, or -1 for error.
	// If an error occurs, subclasses should set the appropriate errno/errstring
	//=
	virtual int virtualread(char *buffer, int length)=0;

	//=
	// Subclasses implement to perform the real finish
	// If an error occurs, subclasses should set the appropriate errno/errstring
	//=
	virtual bool virtualfinish()=0;

	//=
	// Subclasses implement to perform the real bind
	// If an error occurs, subclasses should set the appropriate errno/errstring
	//=
	virtual bool virtualbind()=0;
	
public:

	SOCKET getSocketDescriptor();
	virtual ~Socket_Comm();

	//=
	// Sets the timeout for sending and reading data
	// Setting the timeout to 0 will block instead of timeout.
	//=
	void setTimeout(int secs, int microsecs);

	//=
	// Binds this object to a socket descriptor
	// A socket descriptor must be set before 'read' and 'send' can occur.
	//=
	bool bind(SOCKET &sock);

	//=
	// Closes the communication.
	// Presents subclasses with an opportunity to finalize the communication and reclaim resources
	//=
	bool finish();

	//=
	// Sends a block of data
	// The Socket_Comm object must be bound to a socket first (see bind())
	//=
	int send(const char *data, int length);

	//=
	// Reads a block of data
	// The Socket_Comm object must be bound to a socket first (see bind())
	//=
	int read(char *buffer, int length);

	//=
	// Returns a textual description of the last known error
	//=
	const char *getError() const;
	
	//=
	// Returns the current timeout's 'second' component for sending and reading data
	//=
	int getTimeoutSecs() const;
	
	//=
	// Returns the current timeout's 'micro-second' component for sending and reading data
	//=
	int getTimeoutMicroSecs() const;
	
};


}}

#endif


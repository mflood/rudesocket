// socket_connect.h
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


#ifndef INCLUDED_SOCKET_CONNECT_H
#define INCLUDED_SOCKET_CONNECT_H

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
// Defines an interface for connecting to and disconnecting from a server
// defines abstract interface used by all Socket Objects
// Socket Objects are responsible for connecting to a server
//=
class Socket_Connect{

	std::string d_error;
	std::string d_server;
	int d_port;
	int d_timeoutsecs;
	int d_timeoutmicrosecs;
	Socket_Connect *s_nextchild;
	
protected:

	//=
	// Call this method to send 'real-time' debug information about the status of the connection
	//=
	void setMessage(const char *message);


	//=
	// Sets the error message for the Connect Object.
	//=
	void setError(const char *error);

	//=
	// Constructor
	//=
	Socket_Connect();

	//=
	// Children must implement to perform actual connection
	// this is the workhorse.....
	// clients should check input for void data
	// If an error occurs, client should setError() and return false;
	//=
	virtual bool simpleConnect(SOCKET &s, const char *destip, int port)=0;
	
public:

	//=
	// Destructor
	//=
	virtual ~Socket_Connect();

	// functions
	//
	bool connect(SOCKET &s, const char *destip, int port);

	//=
	// Sets the child connection of this Connect Object.
	// If a child is set, then this object will connect to 
	// This class assumes responsibility for the child object, and will
	// delete the child in the destructor or if the child is replaced.
	//=
	void setChild(Socket_Connect *nextChild);

	//=
	// Sets the server address associated with this connection object (it cascades)
	// Accepts IP Addresses and Server Names
	//=
	void setServer(const char *server);

	//=
	// Sets the port associated with this connection object
	// Should be non-negative int from 0 to 65536
	//=
	void setPort(int port);

	//=
	// Sets the timeout for this connection object and its child connection object
	// 0, 0 = no timeout
	// 1.75 seconds = (1, 75000)
	// default (0, 0);
	//=
	void setTimeout(int seconds, int microseconds);

	//=
	// Returns the child connection if one exists.
	//=
	Socket_Connect *getChild() const;

	//=
	// Returns the last known error for this connection object
	// Will also return errors that originated in children.
	//=
	const char *getError() const;

	//=
	// Returns the server address associated with this connection object
	//=
	const char *getServer() const;

	//= 
	// Returns the server port associated with this connection object.
	//=
	int getPort() const;

	//=
	// Returns the 'seconds' portion of the current timeout value
	//=
	int getTimeoutSecs() const;

	//=
	// Returns the 'milliseconds' portion of the current timeout value
	//=
	int getTimeoutMicroSecs() const;

};
}}
#endif


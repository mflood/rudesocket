// socket_tcpclient_normal.h
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


//
#ifndef INCLUDED_SOCKET_TCPCLIENT_H
#define INCLUDED_SOCKET_TCPCLIENT_H

#include "socket_platform.h"

#ifndef INCLUDED_STRING
#include <string>
#define INCLUDED_STRING
#endif

namespace rude{
namespace sckt{

class Socket_Connect;
class Socket_Comm;

//=
// Coordinates and adds some additional functionality to COMM and CONNECT objects
//=
class Socket_TCPClient{

	int timeoutsecs;
	int timeoutmilli;
   std::string d_readbuffer;
	std::string d_error;
	Socket_Connect *d_connection;
	Socket_Comm *d_comm;
	SOCKET d_sock;

protected:

	void setError(const char *error);
	
public:

	Socket_TCPClient();

	virtual ~Socket_TCPClient();

	Socket_Connect *getConnection()
	{
		return d_connection;
	}
	
	Socket_Comm *getComm()
	{
		return d_comm;
	}

	void setTimeout(int seconds, int microseconds);
	void setConnection(Socket_Connect *connection);
	void setComm(Socket_Comm *comm);

	const char *getError();

	int send(const char *data, int length);
	int read(char *buffer, int length);
	const char *reads();
	const char *readline();
	bool sends(const char *buffer);
	bool connect(const char *server, int port);
	bool close();
};
}}
#endif


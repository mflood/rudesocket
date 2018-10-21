// rude_socketimpl.h
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


#ifndef INCLUDED_RUDE_SOCKETIMPL_H
#define INCLUDED_RUDE_SOCKETIMPL_H

#ifndef INCLUDED_SOCKET_TCPCLIENT_H
#include "socket_tcpclient.h"
#endif

#ifndef INCLUDED_SOCKET_CONNECT_H
#include "socket_connect.h"
#endif


namespace rude{
namespace sckt{

//=
// The implementation of the Rude_Socket Component.
// All requests passed to the Rude_Socket object get forwarded to this object.
//=
class Rude_SocketImpl{

	Socket_TCPClient d_tcpclient;
	Socket_Connect *d_lastconnect;

public:

	//=
	//
	//=
	Rude_SocketImpl();

	//=
	//
	//=
	~Rude_SocketImpl();

	//=
	//
	//=
	void setTimeout(int seconds, int microseconds);

	//=
	//
	//=
	const char *getError();

	//=
	//
	//=
	bool connect(const char *server, int port);

	//=
	//
	//=
	bool connectSSL(const char *server, int port);

	//=
	//
	//=
	bool insertTunnel(const char *server, int port);

	//=
	//
	//=
	bool insertSocks4(const char *server, int port, const char *username);

	//=
	//
	//=
	bool insertSocks5(const char *server, int port, const char *username, const char *password);

	//=
	//
	//=
	bool insertProxy(const char *server, int port);
	
	//=
	//
	//=
	int send(const char *data, int length);

	//=
	//
	//=
	int read(char *buffer, int length);

	//=
	//
	//=
	const char *reads();

	//=
	//
	//=
	const char *readline();

	//=
	//
	//=
	bool sends(const char *buffer);

	//=
	//
	//=
	bool close();
	
	//=
	// Sets an output stream to receive realtime messages about the socket
	//=
	void setMessageStream(std::ostream &o);

};
}}
#endif


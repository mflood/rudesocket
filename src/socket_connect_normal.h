// socket_connect_normal.h
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


#ifndef INCLUDED_SOCKET_CONNECT_NORMAL_H
#define INCLUDED_SOCKET_CONNECT_NORMAL_H

#ifndef INCLUDED_SOCKET_CONNECT_H
#include "socket_connect.h"
#endif

namespace rude{
namespace sckt{

//=
// Establishes a direct socket connection to its destination.
//=
class Socket_Connect_Normal: public Socket_Connect{

protected:

	int connecttimeout(int socket, struct sockaddr *addr, socklen_t len, int msec) ;
	bool simpleConnect(SOCKET &sock, const char *server, int port);

public:

	virtual ~Socket_Connect_Normal();

};
}}
#endif


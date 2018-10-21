// socket_connect_socks4.h
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


#ifndef INCLUDED_SOCKET_CONNECT_SOCKS4_H
#define INCLUDED_SOCKET_CONNECT_SOCKS4_H

#ifndef INCLUDED_SOCKET_CONNECT_H
#include "socket_connect.h"
#endif

namespace rude{
namespace sckt{

class Socket_Comm;

//=
// Connects to its destination through a SOCKS4 server
//=
class Socket_Connect_Socks4: public Socket_Connect{

	std::string d_username;
	char d_buffer[10];
	Socket_Comm *s_socketcomm;

protected:
	bool simpleConnect(SOCKET &s, const char *server, int port);
public:

	Socket_Connect_Socks4();
	~Socket_Connect_Socks4();

	void setUsername(const char *username);
};
}}
#endif


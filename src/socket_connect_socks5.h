// socket_connect_socks5.h
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


#ifndef INCLUDED_SOCKET_CONNECT_SOCKS5_H
#define INCLUDED_SOCKET_CONNECT_SOCKS5_H

#ifndef INCLUDED_SOCKET_CONNECT_H
#include "socket_connect.h"
#endif

#ifndef INCLUDED_SOCKET_COMM_NORMAL_H
#include "socket_comm_normal.h"
#endif

#ifndef INCLUDED_STRING
#include <string>
#define INCLUDED_STRING
#endif


namespace rude{
namespace sckt{

class Socket_Comm;

//=
// Connects to its destination through a SOCKS5 server
//=
class Socket_Connect_Socks5: public Socket_Connect
{

	std::string d_username;
	std::string d_password;
	char d_buffer[200];
	Socket_Comm_Normal d_socketcomm;

protected:
	bool simpleConnect(SOCKET &s, const char *server, int port);
public:

	Socket_Connect_Socks5();
	~Socket_Connect_Socks5();

	void setUsername(const char *username);
	void setPassword(const char *password);

};
}}
#endif


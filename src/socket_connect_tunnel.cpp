// socket_connect_tunnel.cpp
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


#include "socket_connect_tunnel.h"

#ifndef INCLUDED_STDIO_H
#include <stdio.h>
#define INCLUDED_STDIO_H
#endif
namespace rude{
namespace sckt{

bool Socket_Connect_Tunnel::simpleConnect(SOCKET &s, const char *destip, int port)
{
	// tunnels are completely transparent, and have to do
	// no work at all...
	return true;
}
}}


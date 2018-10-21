// socket_comm_normal.h
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


#ifndef INCLUDED_SOCKET_COMM_NORMAL_H
#define INCLUDED_SOCKET_COMM_NORMAL_H

#ifndef INCLUDED_SOCKET_COMM_H
#include "socket_comm.h"
#endif

namespace rude{
namespace sckt{

//=
// Provides communication services through a connected socket
//=
class Socket_Comm_Normal: public Socket_Comm{

protected:

	//=
	// Uses regular socket library and select to perform a timing-out send
	//=
	int virtualsend(const char *data, int length);

	//=
	// Uses regular socket library and select to perform a timing-out read
	//=
	int virtualread(char *buffer, int len);

	//=
	// Doesn't do anything in this implementation
	//=
	bool virtualfinish();

	//=
	// Doesn't do anything in this implementation
	//=
	bool virtualbind();

public:

	Socket_Comm_Normal(){};
	~Socket_Comm_Normal();
};
}}
#endif


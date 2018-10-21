// socket.cpp
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


#include "socket.h"

#ifndef INCLUDED_RUDE_SOCKETIMPL_H
#include "rude_socketimpl.h"
#endif


using namespace rude::sckt;

namespace rude{

Socket::Socket()
{
	d_implementation = new Rude_SocketImpl();
}

Socket::~Socket()
{
	delete d_implementation;
}

void Socket::setTimeout(int seconds, int microseconds)
{
	d_implementation->setTimeout(seconds, microseconds);
}
	
const char *Socket::getError()
{
	return d_implementation->getError();
}

bool Socket::connect(const char *server, int port)
{
	return d_implementation->connect(server, port);
}
	
bool Socket::connectSSL(const char *server, int port)
{
	return d_implementation->connectSSL(server, port);
}

bool Socket::insertTunnel(const char *server, int port)
{
	return d_implementation->insertTunnel(server, port);
}

bool Socket::insertSocks4(const char *server, int port, const char *username)
{
	return d_implementation->insertSocks4(server, port, username);
}

bool Socket::insertSocks5(const char *server, int port, const char *username, const char *password)
{
	return d_implementation->insertSocks5(server, port, username, password);
}
	
bool Socket::insertProxy(const char *server, int port)
{
	return d_implementation->insertProxy(server, port);
}	
	
int Socket::send(const char *data, int length)
{
	return d_implementation->send(data, length);
}
	
int Socket::read(char *buffer, int length)
{
	return d_implementation->read(buffer, length);
}

const char *Socket::reads()
{
	return d_implementation->reads();
}
	
const char *Socket::readline()
{
	return d_implementation->readline();
}

bool Socket::sends(const char *buffer)
{
	return d_implementation->sends(buffer);
}
	
bool Socket::close()
{
	return d_implementation->close();
}

void Socket::setMessageStream(std::ostream &o)
{
	d_implementation->setMessageStream(o);
}
}

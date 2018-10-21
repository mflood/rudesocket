// rude_socketimpl.cpp
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


#include "rude_socketimpl.h"


#ifndef INCLUDED_SOCKET_CONNECT_NORMAL_H
#include "socket_connect_normal.h"
#endif

#ifndef INCLUDED_SOCKET_CONNECT_SOCKS5_H
#include "socket_connect_socks5.h"
#endif

#ifndef INCLUDED_SOCKET_CONNECT_PROXY_H
#include "socket_connect_proxy.h"
#endif

#ifndef INCLUDED_SOCKET_CONNECT_TUNNEL_H
#include "socket_connect_tunnel.h"
#endif

#ifndef INCLUDED_SOCKET_CONNECT_SOCKS4_H
#include "socket_connect_socks4.h"
#endif

#ifndef INCLUDED_SOCKET_COMM_NORMAL_H
#include "socket_comm_normal.h"
#endif

#ifdef USING_OPENSSL
#ifndef INCLUDED_SOCKET_COMM_SSH_H
#include "socket_comm_ssh.h"
#endif
#endif

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

using namespace std;


namespace rude{
namespace sckt{


Rude_SocketImpl::Rude_SocketImpl()
{
	d_lastconnect = 0;
}

Rude_SocketImpl::~Rude_SocketImpl()
{
}

void Rude_SocketImpl::setTimeout(int seconds, int microseconds)
{
	d_tcpclient.setTimeout(seconds, microseconds);
}
	
const char *Rude_SocketImpl::getError()
{
	return d_tcpclient.getError();
}

bool Rude_SocketImpl::connect(const char *server, int port)
{
	Socket_Connect_Normal *normalconnect = new Socket_Connect_Normal();
	normalconnect->setChild(d_lastconnect);
	d_tcpclient.setConnection(normalconnect);
	Socket_Comm_Normal *comm = new Socket_Comm_Normal();
	d_tcpclient.setComm(comm);
	return d_tcpclient.connect(server, port);
}
	
bool Rude_SocketImpl::connectSSL(const char *server, int port)
{
	Socket_Connect_Normal *normalconnect = new Socket_Connect_Normal();
	normalconnect->setChild(d_lastconnect);
	d_tcpclient.setConnection(normalconnect);
	
#ifdef USING_OPENSSL
	Socket_Comm_SSH *comm = new Socket_Comm_SSH();
#else
	Socket_Comm_Normal *comm = new Socket_Comm_Normal();
#endif

	d_tcpclient.setComm(comm);
	return d_tcpclient.connect(server, port);
}

bool Rude_SocketImpl::insertSocks4(const char *server, int port, const char *username)
{
	Socket_Connect_Socks4 *tunnel = new Socket_Connect_Socks4();
	tunnel->setServer(server);
	tunnel->setPort(port);
	tunnel->setUsername(username);

	if(d_lastconnect)
	{
		tunnel->setChild(d_lastconnect);
		d_lastconnect = tunnel;
	}
	else
	{
		d_lastconnect = tunnel;
	}
	return true;
}

bool Rude_SocketImpl::insertTunnel(const char *server, int port)
{
	Socket_Connect_Tunnel *tunnel = new Socket_Connect_Tunnel();
	tunnel->setServer(server);
	tunnel->setPort(port);

	if(d_lastconnect)
	{
		tunnel->setChild(d_lastconnect);
		d_lastconnect = tunnel;

	}
	else
	{
		d_lastconnect = tunnel;
	}
	return true;
}

bool Rude_SocketImpl::insertSocks5(const char *server, int port, const char *username, const char *password)
{
	Socket_Connect_Socks5 *socks = new Socket_Connect_Socks5();
	socks->setServer(server);
	socks->setPort(port);
	socks->setUsername(username);
	socks->setPassword(password);

	if(d_lastconnect)
	{
		socks->setChild(d_lastconnect);
		d_lastconnect = socks;

	}
	else
	{
		d_lastconnect = socks;
	}
	return true;
}
	
bool Rude_SocketImpl::insertProxy(const char *server, int port)
{
	Socket_Connect_Proxy *proxy = new Socket_Connect_Proxy();
	proxy->setServer(server);
	proxy->setPort(port);

	if(d_lastconnect)
	{
		proxy->setChild(d_lastconnect);
		d_lastconnect = proxy;

	}
	else
	{
		d_lastconnect = proxy;
	}
	return true;
}	
	
int Rude_SocketImpl::send(const char *data, int length)
{
	return d_tcpclient.send(data, length);
}
	
int Rude_SocketImpl::read(char *buffer, int length)
{
	return d_tcpclient.read(buffer, length);
}

const char *Rude_SocketImpl::reads()
{
	return d_tcpclient.reads();
}
	
const char *Rude_SocketImpl::readline()
{
	return d_tcpclient.readline();
}

bool Rude_SocketImpl::sends(const char *buffer)
{
	return d_tcpclient.sends(buffer);
}
	
bool Rude_SocketImpl::close()
{
	return d_tcpclient.close();
}

void Rude_SocketImpl::setMessageStream( std::ostream &o)
{
	// not implemented yet
}


}}

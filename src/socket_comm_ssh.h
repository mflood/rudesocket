// socket_comm_ssh.h
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

#ifdef USING_OPENSSL

#ifndef INCLUDED_SOCKET_COMM_SSH_H
#define INCLUDED_SOCKET_COMM_SSH_H

#ifndef INCLUDED_SOCKET_COMM_H
#include "socket_comm.h"
#endif

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h> 

namespace rude{
namespace sckt{

//=
// Initializes and provides <i><b>secure</b></i> communication services through a connected socket
//=
class Socket_Comm_SSH: public Socket_Comm{

	SSL_CTX *ctx;
	SSL *ssl;

protected:

	virtual int virtualsend(const char *data, int length);
	virtual int virtualread(char *buffer, int length);
	virtual bool virtualfinish();
	virtual bool virtualbind();

public:

	Socket_Comm_SSH();
	~Socket_Comm_SSH();
};
}}
#endif 
#endif



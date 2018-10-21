// socket_comm_ssh.cpp
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

#include "socket_comm_ssh.h"

#ifndef INCLUDED_IOSTREAM
#include <iostream>
#define INCLUDED_IOSTREAM
#endif

using namespace std;

namespace rude{
namespace sckt{

Socket_Comm_SSH::Socket_Comm_SSH()
{
	// initialize SSL stuff
	//
	SSL_library_init();
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
};

Socket_Comm_SSH::~Socket_Comm_SSH()
{
	this->finish();
}

int Socket_Comm_SSH::virtualsend(const char *data, int length)
{
	int err=SSL_write(ssl, data, length);
	return err;
}

bool Socket_Comm_SSH::virtualfinish()
{
	    SSL_shutdown(ssl);
       SSL_free(ssl);
       SSL_CTX_free(ctx);
		 return true;
}


bool Socket_Comm_SSH::virtualbind()
{
	int err;


	// build the SSL objects...
	//
	ctx=SSL_CTX_new(SSLv23_client_method());

	if (!ctx) 
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	ssl=SSL_new(ctx);
	if (!ssl)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	// assign the socket you created for SSL to use
	//
	SSL_set_fd(ssl, getSocketDescriptor());

	// communicate!!
	/////////////////////////////////////////////
	err=SSL_connect(ssl);
	if(err == -1)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	//printf ("SSL connection using %s\n", SSL_get_cipher(ssl));
	return (true);
}

int Socket_Comm_SSH::virtualread(char *buffer, int length)
{
	int read_size;
   read_size=SSL_read(ssl, buffer, length);
	return read_size;
}

}}

#endif



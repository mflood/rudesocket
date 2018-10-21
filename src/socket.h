// socket.h
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


#ifndef INCLUDED_RUDE_SOCKET_H
#define INCLUDED_RUDE_SOCKET_H

#ifndef INCLUDED_STDIO_H
#include <iostream>
#define INCLUDED_STDIO_H
#endif

namespace rude{

namespace sckt{
class Rude_SocketImpl;
}


//!The public interface to the Socket component.
/*!

	If you are using windows, you will need to initiate the winsock DLL yourself, 
	and finish the DLL yourself when you are done using the component.

	\section usage General Usage

	\code
	Socket *socket = new Socket();
	socket->connect("google.com", 80);
	socket->sends("GET / HTTP/1.0\n\n");
	const char *response = socket->reads();
	cout << response;
	socket->close();
	\endcode

	\section sslusage SSL Usage
	\code
	Socket *socket = new Socket();
	socket->connectSSL("google.com", 443);
	socket->sends("GET / HTTP/1.0\n\n");
	const char *response = socket->reads();
	cout << response;
	socket->close();
	\endcode

	\section chaining Chaining Connections
	\code
	Socket *socket = new Socket();
	socket->insertSocks4("12.34.56.78", 8000, "username");
	socket->insertSocks5("12.34.56.78", 8000, "username", "password");
	socket->insertProxy("12.34.56.78", 8080);
	socket->connectSSL("google.com", 443);
	socket->sends("GET / HTTP/1.0\n\n");
	const char *response = socket->reads();
	cout << response;
	socket->close();
	\endcode

	\section error Adding Error checking
	\code
	Socket *socket = new Socket();
	if(socket->connectSSL("google.com", 443))
	{
		if(socket->sends("GET / HTTP/1.0\n\n"))
		{
			const char *response = socket->reads();
			if(response)
			{
				cout << response;
			}
			else
			{
				cout << socket->getError() << "\n";
			}
		}
		else
		{
			cout << socket->getError() << "\n";
		}
		socket->close();
	}
	else
	{
		cout << socket->getError() << "\n";
	}
	\endcode

*/
class Socket{

	sckt::Rude_SocketImpl *d_implementation;

public:

	//! Constructor
	Socket();


	//! Destructor
	~Socket();

	//! Sets the timeout value for Connect, Read and Send operations.
	/*! 
		Setting the timeout to 0 removes the timeout - making the Socket blocking.
	*/
	void setTimeout(int seconds, int microseconds);

	//! Returns a description of the last known error
	const char *getError();

	//! Connects to the specified server and port
	/*!
		 If proxies have been specified, the connection passes through tem first.
	*/
	bool connect(const char *server, int port);

	//! Connects to the specified server and port over a secure connection
	/*!
		 If proxies have been specified, the connection passes through them first.
	*/
	bool connectSSL(const char *server, int port);

	//! Inserts a transparent tunnel into the connect chain
	/*!
		A transparent Tunnel is a server that accepts a connection on a certain port,
		and always connects to a particular server:port address on the other side.
		Becomes the last server connected to in the chain before connecting to the destination server
	*/
	bool insertTunnel(const char *server, int port);

	//! Inserts a Socks5 server into the connect chain
	/*!
		Becomes the last server connected to in the chain before connecting to the destination server
	*/
	bool insertSocks5(const char *server, int port, const char *username, const char *password);

	//! Inserts a Socks4 server into the connect chain
	/*!
		Becomes the last server connected to in the chain before connecting to the destination server
	*/
	bool insertSocks4(const char *server, int port, const char *username);

	//! Inserts a CONNECT-Enabled HTTP proxy into the connect chain
	/*!
		Becomes the last server connected to in the chain before connecting to the destination server
	*/
	bool insertProxy(const char *server, int port);
	
	//! Sends a buffer of data over the connection
	/*!
		 A connection must established before this method can be called
	*/
	int send(const char *data, int length);

	//! Reads a buffer of data from the connection
	/*!
		A connection must established before this method can be called
	*/
	int read(char *buffer, int length);

	//! Reads everything available from the connection
	/*!
		A connection must established before this method can be called
	*/
	const char *reads();

	//! Reads a line from the connection
	/*!
		A connection must established before this method can be called
	*/
	const char *readline();

	//! Sends a null terminated string over the connection
	/*!
		The string can contain its own newline characters.
		Returns false and sets the error message if it fails to send the line.
		A connection must established before this method can be called
	*/
	bool sends(const char *buffer);

	//! Closes the connection
	/*!
		 A connection must established before this method can be called
	*/
	bool close();

	//! Sets an output stream to receive real-time messages about the socket
	void setMessageStream(std::ostream &o);

};
}
#endif

/*!
	/manonly
.BI	License
	/manonly
	
*/

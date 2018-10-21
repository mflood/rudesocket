// socket_platform.h
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


#ifndef INCLUDED_PLATFORM_H
#define INCLUDED_PLATFORM_H


#ifdef WIN32

///////PUT THIS AT THE BEGINNING OF MAIN()
////#ifdef WIN32
////WSADATA wsadata;
////WSAStartup(MAKEWORD(2,2), &wsadata);
////#endif
////
////////PUT THIS AT THE END OF MAIN()
////#ifdef WIN32
////WSACleanup();
////#endif

#ifndef INCLUDED_WINDOWS_H
#include <windows.h>
#define INCLUDED_WINDOWS_H
#endif

#ifndef INCLUDED_WINSOCK2_H
#include <winsock2.h>
#define INCLUDED_WINSOCK2_H
#endif

#define socklen_t int


#else

// LINUX
#include <sys/socket.h> // for connect() socket() and bind()
#include <sys/time.h> // for timeval{}
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h> // for close(socket)
#define SOCKET int
#define LPHOSTENT hostent*


#endif

#endif

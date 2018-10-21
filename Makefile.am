# Makefile.am
#
# Copyright (C) 2002, 2003, 2004, 2005 Matthew Flood
# See file AUTHORS for contact information
#
# This file is part of RudeSocket.
#
# RudeSocket is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# RudeSocket is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with RudeSocket; (see COPYING) if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#------------------------------------------------------------------------

lib_LTLIBRARIES = librudesocket.la

librudesocket_la_SOURCES = 	src/rude_socketimpl.cpp \
	src/rude_socketimpl.h \
	src/socket.cpp \
	src/socket.h \
	src/socket_comm.cpp \
	src/socket_comm.h \
	src/socket_comm_normal.cpp \
	src/socket_comm_normal.h \
	src/socket_comm_ssh.cpp \
	src/socket_comm_ssh.h \
	src/socket_connect.cpp \
	src/socket_connect.h \
	src/socket_connect_normal.cpp \
	src/socket_connect_normal.h \
	src/socket_connect_proxy.cpp \
	src/socket_connect_proxy.h \
	src/socket_connect_socks4.cpp \
	src/socket_connect_socks4.h \
	src/socket_connect_socks5.cpp \
	src/socket_connect_socks5.h \
	src/socket_connect_tunnel.cpp \
	src/socket_connect_tunnel.h \
	src/socket_platform.h \
	src/socket_tcpclient.cpp \
	src/socket_tcpclient.h

librudesocket_la_LDFLAGS = -version-info 1:2:0
librudesocket_la_LIBADD = -lssl


pkginclude_HEADERS = src/socket.h
pkgincludedir = $(includedir)/rude

man_MANS = man3/rudesocket.3

EXTRA_DIST = $(man_MANS)


AUTOMAKE_OPTIONS = gnits dist-bzip2 dist-shar dist-zip



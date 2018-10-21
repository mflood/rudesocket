# Process this file with autoconf to produce a configure script.
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

AC_INIT(RudeSocket, 1.2.0, matt@rudeserver.com, rudesocket)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE(rudesocket, 1.2.0)



# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADERS([arpa/inet.h netdb.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname select socket strerror])


AC_DEFUN([CHECK_OPENSSL],
[AC_MSG_CHECKING(if openssl is wanted)
AC_ARG_WITH(openssl,
[  --with-openssl enable SSL support [will use openssl] 
],
[   AC_MSG_RESULT(yes)

	AC_CHECK_LIB(ssl, main,, AC_MSG_ERROR(RudeSocket requires libssl from http://www.openssl.org/))
	printf "Using openssl";
   CFLAGS="$CFLAGS -DUSING_OPENSSL";
   CXXFLAGS="$CXXFLAGS -DUSING_OPENSSL";
	LIBS="$LIBS -lssl";
	USING_OPENSSL=yes
	AC_SUBST(USING_OPENSSL)
],
[
    AC_MSG_RESULT(no)
])
])dnl


CHECK_OPENSSL()

AC_OUTPUT(Makefile )



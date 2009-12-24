dnl Copyright (C) 1999-2001 Open Source Telecom Corporation.
dnl  
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software 
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl 
dnl As a special exception to the GNU General Public License, if you 
dnl distribute this file as part of a program that contains a configuration 
dnl script generated by Autoconf, you may include it under the same 
dnl distribution terms that you use for the rest of that program.

AC_DEFUN([OST_CC_TYPES],[
	AC_REQUIRE([OST_SYS_POSIX])
	AC_CHECK_HEADERS(sys/types.h bits/wordsize.h)
	AC_EGREP_HEADER(u_int8_t,sys/types.h,[
		AC_DEFINE(HAVE_SYS_TYPES_STD, [1], [have systypes])
		AC_EGREP_HEADER(u_int64_t,sys/types.h,[
			AC_DEFINE(HAVE_SYS_TYPES_64, [1], [have 64 bit longs])
			])	
		])
	AC_EGREP_HEADER(long long,sys/types.h,[
		AC_DEFINE(HAVE_LONG_LONG, [1], [have long longs])
	])
AH_TOP([

#undef HAVE_SYS_TYPES_STD
#undef HAVE_SYS_TYPES_64
#undef HAVE_LONG_LONG
#undef HAVE_SYS_TYPES

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef	HAVE_BITS_WORSIZE_H
#include <bits/wordtypes.h>
#endif

#ifdef HAVE_SYS_TYPES_STD
typedef int8_t int8;
typedef u_int8_t uint8;
typedef int16_t int16;
typedef u_int16_t uint16;
typedef int32_t int32;
typedef u_int32_t uint32;
#	ifdef HAVE_SYS_TYPES_64
#		define HAVE_64_BITS
typedef int64_t int64;
typedef u_int64_t uint64;
#	endif
#elif !defined(__sun) && !defined(__SUN__)
typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
#endif

#ifndef HAVE_SYS_TYPES_64
#if defined(__WORDSIZE) || defined(__arch64__)
#if __WORDSIZE >= 64 || defined(__arch64__)
typedef long int int64;
typedef unsigned long int uint64;
#define	HAVE_SYS_TYPES_64
#define	HAVE_64_BITS
#endif
#endif
#endif

#ifndef	HAVE_SYS_TYPES_64
#ifdef	__GNUC__
#if defined(HAVE_LONG_LONG) || defined(_LONGLONG)
__extension__
typedef long long int int64;
__extension__
typedef unsigned long long int uint64;
#define	HAVE_SYS_TYPES_64
#define	HAVE_64_BITS
#endif
#endif
#endif

#ifndef HAVE_SYS_TYPES_64
#if defined(HAVE_LONG_LONG) || defined(_LONGLONG)
#define HAVE_64_BITS
typedef long long int64;
typedef unsigned long long uint64;
#endif
#endif
	])

])


dnl $Id: libvorbis.m4 735 2008-11-01 18:34:34Z mgrimm $

dnl # Check for working installations of libvorbis, libvorbisfile and
dnl # libvorbisenc.
dnl # Provides appropriate --with configuration options, fills and substitutes
dnl # the VORBIS_CFLAGS, VORBIS_CPPFLAGS, VORBIS_LDFLAGS, LIBVORBIS_LIBS,
dnl # LIBVORBISENC_LIBS and LIBVORBISFILE_LIBS variables accordingly.


dnl # Copyright (c) 2008 Moritz Grimm <mgrimm@mrsserver.net>
dnl #
dnl # Permission to use, copy, modify, and distribute this software for any
dnl # purpose with or without fee is hereby granted, provided that the above
dnl # copyright notice and this permission notice appear in all copies.
dnl #
dnl # THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
dnl # WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
dnl # MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
dnl # ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
dnl # WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
dnl # ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
dnl # OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


dnl # Each of these macros supply VORBIS_CFLAGS, VORBIS_CPPFLAGS and
dnl # VORBIS_LDFLAGS.

dnl # For LIBVORBIS_LIBS:
dnl # AX_CHECK_LIBVORBIS([VORBISLIBS-VERSION], [ACTION-IF-FOUND],
dnl #     [ACTION-IF-NOT-FOUND])

dnl # For LIBVORBISENC_LIBS:
dnl # AX_CHECK_LIBVORBISENC([VORBISLIBS-VERSION], [ACTION-IF-FOUND],
dnl #     [ACTION-IF-NOT-FOUND])

dnl # For LIBVORBISFILE_LIBS:
dnl # AX_CHECK_LIBVORBISFILE([VORBISLIBS-VERSION], [ACTION-IF-FOUND],
dnl #     [ACTION-IF-NOT-FOUND])


AC_DEFUN([_AX_CHECK_VORBIS_OPTS],
[
AC_REQUIRE([PKG_PROG_PKG_CONFIG])
AC_ARG_VAR([VORBIS_CFLAGS],
	[C compiler flags for the Vorbis libraries])
AC_ARG_VAR([VORBIS_CPPFLAGS],
	[C preprocessor flags for the Vorbis libraries])
AC_ARG_VAR([VORBIS_LDFLAGS],
	[linker flags for the Vorbis libraries])
if test x"${prefix}" = "xNONE"; then
	ax_check_libvorbis_prefix="/usr/local"
else
	ax_check_libvorbis_prefix="${prefix}"
fi
if test -z "${VORBIS_CFLAGS}"; then
	VORBIS_CFLAGS="`${PKG_CONFIG} --cflags-only-other vorbis`"
fi
if test -z "${VORBIS_CPPFLAGS}"; then
	VORBIS_CPPFLAGS="`${PKG_CONFIG} --cflags-only-I vorbis`"
fi
if test -z "${VORBIS_LDFLAGS}"; then
	VORBIS_LDFLAGS="\
		`${PKG_CONFIG} --libs-only-L vorbis` \
		`${PKG_CONFIG} --libs-only-other vorbis` \
	"
fi
])

AC_DEFUN([AX_CHECK_LIBVORBIS],
[
AC_REQUIRE([_AX_CHECK_VORBIS_OPTS])
AC_ARG_VAR([LIBVORBIS_LIBS],
	[libraries to use for libvorbis])
AC_CACHE_VAL([local_cv_have_lib_libvorbis],
[
local_cv_have_lib_libvorbis=no

PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${ax_check_libogg_prefix}/lib/pkgconfig"
if test -z "${PKG_CONFIG}"; then
	AC_MSG_ERROR([The pkg-config utility is required.], [1])
fi

dnl ####### BEGIN CHECK ######
PKG_CHECK_EXISTS([vorbis $1], [
dnl ##########################

libvorbis_libs_autodetect=no
if test -z "${LIBVORBIS_LIBS}"; then
	LIBVORBIS_LIBS="`${PKG_CONFIG} --libs-only-l vorbis`"
	libvorbis_libs_autodetect=yes
fi

ax_check_libvorbis_save_CFLAGS="${CFLAGS}"
ax_check_libvorbis_save_CPPFLAGS="${CPPFLAGS}"
ax_check_libvorbis_save_LDFLAGS="${LDFLAGS}"
ax_check_libvorbis_save_LIBS="${LIBS}"
AC_LANG_PUSH([C])
CFLAGS="${CFLAGS} ${VORBIS_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${VORBIS_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${VORBIS_LDFLAGS}"
LIBS="${LIBVORBIS_LIBS} ${LIBS}"
AC_CHECK_HEADER([vorbis/codec.h],
[
	AC_MSG_CHECKING([if libvorbis works])
	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM(
			[[
			  #include <stdlib.h>
			  #include <ogg/ogg.h>
			  #include <vorbis/codec.h>
			]],
			[[
			  vorbis_info_init(NULL);
			]]
		)],
		[
		  AC_MSG_RESULT([yes])
		  local_cv_have_lib_libvorbis=yes
		],
		[
		  AC_MSG_RESULT([no])
		]
	)
	AC_CHECK_TYPE([struct ovectl_ratemanage_arg],
		[],
		[AC_MSG_ERROR([These Vorbis libraries are too old, please upgrade.], [1])],
		[
		  #include <stdlib.h>
		  #include <ogg/ogg.h>
		  #include <vorbis/codec.h>
		  #include <vorbis/vorbisenc.h>
		]
	)
])
CFLAGS="${ax_check_libvorbis_save_CFLAGS}"
CPPFLAGS="${ax_check_libvorbis_save_CPPFLAGS}"
LDFLAGS="${ax_check_libvorbis_save_LDFLAGS}"
LIBS="${ax_check_libvorbis_save_LIBS}"
AC_LANG_POP([C])

dnl ####### END CHECK ########
], [])
dnl ##########################

])

AC_MSG_CHECKING([for libvorbis $1])
if test x"${local_cv_have_lib_libvorbis}" = "xyes"; then
	AC_MSG_RESULT([yes])
	:
	$2
else
	AC_MSG_RESULT([no])
	:
	$3
fi

])

AC_DEFUN([AX_CHECK_LIBVORBISFILE],
[
AC_REQUIRE([_AX_CHECK_VORBIS_OPTS])
AC_ARG_VAR([LIBVORBISFILE_LIBS],
	[libraries to use for libvorbisfile])
AC_CACHE_VAL([local_cv_have_lib_libvorbisfile],
[
local_cv_have_lib_libvorbisfile=no

PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${ax_check_libogg_prefix}/lib/pkgconfig"
if test -z "${PKG_CONFIG}"; then
	AC_MSG_ERROR([The pkg-config utility is required.], [1])
fi

dnl ####### BEGIN CHECK ######
PKG_CHECK_EXISTS([vorbisfile $1], [
dnl ##########################

libvorbisfile_libs_autodetect=no
if test -z "${LIBVORBISFILE_LIBS}"; then
	LIBVORBISFILE_LIBS="`${PKG_CONFIG} --libs-only-l vorbisfile`"
	libvorbisfile_libs_autodetect=yes
fi

ax_check_libvorbisfile_save_CFLAGS="${CFLAGS}"
ax_check_libvorbisfile_save_CPPFLAGS="${CPPFLAGS}"
ax_check_libvorbisfile_save_LDFLAGS="${LDFLAGS}"
ax_check_libvorbisfile_save_LIBS="${LIBS}"
AC_LANG_PUSH([C])
CFLAGS="${CFLAGS} ${VORBIS_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${VORBIS_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${VORBIS_LDFLAGS}"
LIBS="${LIBVORBISFILE_LIBS} ${LIBS}"
AC_CHECK_HEADER([vorbis/vorbisfile.h],
[
	AC_MSG_CHECKING([if libvorbisfile works])
	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM(
			[[
			  #include <stdlib.h>
			  #include <vorbis/vorbisfile.h>
			]],
			[[
			  ov_open(NULL, NULL, NULL, 0);
			]]
		)],
		[
		  AC_MSG_RESULT([yes])
		  local_cv_have_lib_libvorbisfile=yes
		],
		[
		  AC_MSG_RESULT([no])
		]
	)
])
CFLAGS="${ax_check_libvorbisfile_save_CFLAGS}"
CPPFLAGS="${ax_check_libvorbisfile_save_CPPFLAGS}"
LDFLAGS="${ax_check_libvorbisfile_save_LDFLAGS}"
LIBS="${ax_check_libvorbisfile_save_LIBS}"
AC_LANG_POP([C])

dnl ####### END CHECK ########
], [])
dnl ##########################

])

AC_MSG_CHECKING([for libvorbisfile $1])
if test x"${local_cv_have_lib_libvorbisfile}" = "xyes"; then
	AC_MSG_RESULT([yes])
	:
	$2
else
	AC_MSG_RESULT([no])
	:
	$3
fi

])

AC_DEFUN([AX_CHECK_LIBVORBISENC],
[
AC_REQUIRE([_AX_CHECK_VORBIS_OPTS])
AC_ARG_VAR([LIBVORBISENC_LIBS],
	[libraries to use for libvorbisenc])
AC_CACHE_VAL([local_cv_have_lib_libvorbisenc],
[
local_cv_have_lib_libvorbisenc=no

PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${ax_check_libogg_prefix}/lib/pkgconfig"
if test -z "${PKG_CONFIG}"; then
	AC_MSG_ERROR([The pkg-config utility is required.], [1])
fi

dnl ####### BEGIN CHECK ######
PKG_CHECK_EXISTS([vorbisenc $1], [
dnl ##########################

libvorbisenc_libs_autodetect=no
if test -z "${LIBVORBISENC_LIBS}"; then
	LIBVORBISENC_LIBS="`${PKG_CONFIG} --libs-only-l vorbisenc`"
	libvorbisenc_libs_autodetect=yes
fi

ax_check_libvorbisenc_save_CFLAGS="${CFLAGS}"
ax_check_libvorbisenc_save_CPPFLAGS="${CPPFLAGS}"
ax_check_libvorbisenc_save_LDFLAGS="${LDFLAGS}"
ax_check_libvorbisenc_save_LIBS="${LIBS}"
AC_LANG_PUSH([C])
CFLAGS="${CFLAGS} ${VORBIS_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${VORBIS_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${VORBIS_LDFLAGS}"
LIBS="${LIBVORBISENC_LIBS} ${LIBS}"
AC_CHECK_HEADER([vorbis/vorbisenc.h],
[
	AC_MSG_CHECKING([if libvorbisenc works])
	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM(
			[[
			  #include <stdlib.h>
			  #include <ogg/ogg.h>
			  #include <vorbis/codec.h>
			  #include <vorbis/vorbisenc.h>
			]],
			[[
			  vorbis_encode_init(NULL, 0, 0, 0, 0, 0);
			]]
		)],
		[
		  AC_MSG_RESULT([yes])
		  local_cv_have_lib_libvorbisenc=yes
		],
		[
		  AC_MSG_RESULT([no])
		]
	)
])
CFLAGS="${ax_check_libvorbisenc_save_CFLAGS}"
CPPFLAGS="${ax_check_libvorbisenc_save_CPPFLAGS}"
LDFLAGS="${ax_check_libvorbisenc_save_LDFLAGS}"
LIBS="${ax_check_libvorbisenc_save_LIBS}"
AC_LANG_POP([C])

dnl ####### END CHECK ########
], [])
dnl ##########################

])

AC_MSG_CHECKING([for libvorbisenc $1])
if test x"${local_cv_have_lib_libvorbisenc}" = "xyes"; then
	AC_MSG_RESULT([yes])
	:
	$2
else
	AC_MSG_RESULT([no])
	:
	$3
fi

])

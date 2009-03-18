/*	$Id$	*/
/*
 * Copyright (c) 2007, 2009 Moritz Grimm <mdgrimm@gmx.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __COMPAT_H__
#define __COMPAT_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else /* HAVE_SYS_TIME_H */
# include <time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#ifdef HAVE_PATHS_H
# include <paths.h>
#endif /* HAVE_PATHS_H */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef _PATH_DEVNULL
# ifdef WIN32
#  define _PATH_DEVNULL 	"nul"
# else /* WIN32 */
#  define _PATH_DEVNULL 	"/dev/null"
# endif /* WIN32 */
#endif /* !_PATH_DEVNULL */

#ifdef WIN32
# include <windows.h>

# define pclose 	_pclose
# define popen		_popen
# define snprintf	_snprintf
# define stat		_stat
# define strncasecmp	strnicmp
# ifndef __GNUC__
#  define strtoll	_strtoi64
# endif /* !__GNUC__ */

# define S_IRGRP	0
# define S_IROTH	0
# define S_IWGRP	0
# define S_IWOTH	0
# define S_IXGRP	0
# define S_IXOTH	0

# define sleep(a)	Sleep((a) * 1000)
#endif /* WIN32 */

#ifndef HAVE_STRUCT_TIMEVAL
struct timeval {
	long	tv_sec;
	long	tv_usec;
};
#endif

/*
 * For compat.c and getopt.c:
 */

extern int	 opterr;
extern int	 optind;
extern int	 optopt;
extern int	 optreset;
extern char	*optarg;

extern int
	local_getopt(int, char * const *, const char *);
extern char *
	local_basename(const char *);

#endif /* __COMPAT_H__ */

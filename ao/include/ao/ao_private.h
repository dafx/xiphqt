/*
 *
 *  ao_private.c
 *
 *       Copyright (C) Stan Seibert - July 2001
 *
 *  This file is part of libao, a cross-platform audio output library.  See
 *  README for a history of this source code.
 *
 *  libao is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  libao is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __AO_PRIVATE_H__
#define __AO_PRIVATE_H__

/* --- Operating System Compatibility --- */

/* 
  OpenBSD systems with a.out binaries require dlsym()ed symbols to be
  prepended with an underscore, so we need the following nasty #ifdef
  hack.
*/
#if defined(__OpenBSD__) && !defined(__ELF__)
#define dlsym(h,s) dlsym(h, "_" s)
#endif

/* RTLD_NOW is the preferred symbol resolution behavior, but
 * some platforms do not support it.  The autoconf script will have
 * already defined DLOPEN_FLAG if the default is unacceptable on the
 * current platform.
 *
 * ALSA requires RTLD_GLOBAL.
 */
#if !defined(DLOPEN_FLAG)
#define DLOPEN_FLAG (RTLD_NOW | RTLD_GLOBAL)
#endif

/* --- Constants --- */

#ifndef AO_SYSTEM_CONFIG
#define AO_SYSTEM_CONFIG "/etc/libao.conf"
#endif
#ifndef AO_USER_CONFIG
#define AO_USER_CONFIG   "/.libao"
#endif

/* --- Structures --- */

typedef struct ao_config {
	char *default_driver;
} ao_config;

struct ao_device {
	int  type; /* live output or file output? */
	int  driver_id;
	ao_functions *funcs;
	FILE *file; /* File for output if this is a file driver */

  /* input not necessarily == output. Right now, byte order and
     channel mappings may be altered. */

	int  client_byte_format;
	int  machine_byte_format;
	int  driver_byte_format;
	char *swap_buffer;
	int  swap_buffer_size; /* Bytes allocated to swap_buffer */

        int input_channels;
        int output_channels;
        int bytewidth;
        char *output_matrix;
        int  *permute_channels;
	void *internal; /* Pointer to driver-specific data */

        int verbose;
};

struct ao_functions {
	int (*test)(void);
	ao_info* (*driver_info)(void);
	int (*device_init)(ao_device *device);
	int (*set_option)(ao_device *device, const char *key,
			  const char *value);
	int (*open)(ao_device *device, ao_sample_format *format);
	int (*play)(ao_device *device, const char *output_samples,
			   uint_32 num_bytes);
	int (*close)(ao_device *device);
	void (*device_clear)(ao_device *device);
	char* (*file_extension)(void);
};

/* --- Functions --- */

void read_config_files (ao_config *config);
int read_config_file(ao_config *config, const char *config_file);

#endif /* __AO_PRIVATE_H__ */

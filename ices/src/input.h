/* input.h
 * - Input functions
 *
 * $Id: input.h,v 1.4.2.1 2002/02/07 09:11:11 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __INPUT_H__
#define __INPUT_H__

#include <shout/shout.h>
#include <vorbis/codec.h>

#include "config.h"
#include "stream.h"
#include "reencode.h"
#include "encode.h"

void input_loop(void);
void input_flush_queue(buffer_queue *queue, int keep_critical);
void *ices_instance_output(void *arg);

#endif /* __INPUT_H__ */



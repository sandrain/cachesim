/* Copyright (C) Hyogi Sim <hyogi@cs.vt.edu>
 * 
 * ---------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
#include <stdlib.h>
#include <string.h>

#include "device.h"

struct stdev *stdev_init(int type,
			__u32 read_latency, __u32 write_latency,
			__u32 block_size, __u64 block_count,
			struct stdev_ops *ops, void *private)
{
	struct stdev *self = malloc(sizeof(*self));

	if (self) {
		self->type = type;
		self->read_latency = read_latency;
		self->write_latency = write_latency;
		self->block_size = block_size;
		self->block_count = block_count;
		self->ops = ops;
		self->private = private;
	}
	return self;
}

void stdev_exit(struct stdev *self)
{
	if (self)
		free(self);
}

struct stdev *ssd_init(__u32 read_latency, __u32 write_latency,
			__u32 block_size, __u64 block_count,
			struct stdev_ops *ops)
{
	struct stdev *self = stdev_init(STDEV_SSD,
					read_latency, write_latency,
					block_size, block_count,
					ops, NULL);
	return self;
}


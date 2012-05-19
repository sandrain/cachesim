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
#include "device.h"

struct storage *storage_init(__u32 node,
			__u64 block_size, __u64 block_count,
			__u32 latency_read, __u32 latency_write,
			int wear,
			struct storage_operations *ops)
{
	struct storage *self = NULL;
	size_t len = sizeof(*self);

	if (wear)
		len += sizeof(__u64) * block_count;

	self = malloc(len);
	if (self) {
		self->node = node;
		self->block_size = block_size;
		self->block_count = block_count;
		self->capacity = block_size * block_count;
		self->latency_read = latency_read;
		self->latency_write = latency_write;
		self->stat_reads = self->stat_writes = 0;
		self->ops = ops;

		if (!wear)
			self->wear = NULL;
	}

	return self;
}


void storage_exit(struct storage *self)
{
	if (self)
		free(self);
}

static __u64 generic_read_block(struct storage *self, __u64 offset, __u64 len)
{
	self->stat_reads += len;

	return self->latency_read * len;
}

static __u64 generic_write_block(struct storage *self, __u64 offset, __u64 len)
{
	self->stat_writes += len;

	return self->latency_write * len;
}

struct storage_operations generic_storage_ops = {
	.read_block	= &generic_read_block,
	.write_block	= &generic_write_block,
};


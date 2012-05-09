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
			double read_latency, double write_latency,
			__u32 block_size, __u64 block_count,
			struct stdev_ops *ops, void *private)
{
	struct stdev *self = malloc(sizeof(*self));

	memset(self, 0, sizeof(*self));

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
	if (self) {
		if (self->private)
			free(self->private);
		free(self);
	}
}

double stdev_get_read_latency(struct stdev *self)
{
	if (!self)
		return -1;

	return self->read_latency;
}

double stdev_get_write_latency(struct stdev *self)
{
	if (!self)
		return -1;

	return self->write_latency;
}

int stdev_get_stat(struct stdev *self, struct stdev_stat *stat)
{
	if (!self || !stat)
		return -1;

	*stat = self->stat;
	return 0;
}

int stdev_read_block(struct stdev *self, __u32 req_node,
			__u64 offset, __u64 count, double *latency)
{
	if (!self)
		return -1;
	if (offset + count > self->block_count)
		return -2;

	self->stat.read_count++;
	self->stat.read_blocks += count;
	*latency = self->read_latency;
	if (self->network)
		*latency += network_get_cost(self->network, self->node,
						req_node);

	return 0;
}

int stdev_write_block(struct stdev *self, __u32 req_node,
			__u64 offset, __u64 count, double *latency)
{
	if (!self)
		return -1;
	if (offset + count > self->block_count)
		return -2;

	self->stat.write_count++;
	self->stat.written_blocks += count;
	*latency = self->write_latency;
	if (self->network)
		*latency += network_get_cost(self->network, self->node,
						req_node);

	return 0;
}

struct stdev *stdev_ssd_init(double read_latency, double write_latency,
				__u32 block_size, __u64 block_count,
				__u32 flash_block_size)
{
	struct stdev *self;
	struct stdev_ssd_stat *ssd_stat;
	size_t size = sizeof(*ssd_stat) +
		sizeof(__u64) * (block_size * block_count) / flash_block_size;

	ssd_stat = malloc(size);
	if (ssd_stat == NULL)
		return NULL;

	memset(ssd_stat, 0, size);
	ssd_stat->flash_block_size = flash_block_size;
	
	self = stdev_init(STDEV_SSD, read_latency, write_latency,
			block_size, block_count, &ssd_dev_ops, ssd_stat);

	return self;
}

static int stdev_ssd_write_block(struct stdev *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency)
{
	__u64 i;
	struct stdev_ssd_stat *stat = (struct stdev_ssd_stat *) self->private;

	for (i = offset; i < offset + count; i++) {
		__u64 index = offset * self->block_size /
			stat->flash_block_size;
		stat->write_dist[index]++;
	}

	return stdev_write_block(self, req_node, offset, count, latency);
}

struct stdev_ops generic_stdev_ops = {
	.get_read_latency	= stdev_get_read_latency,
	.get_write_latency	= stdev_get_write_latency,
	.get_stat		= stdev_get_stat,
	.read_block		= stdev_read_block,
	.write_block		= stdev_write_block,
};

struct stdev_ops ssd_dev_ops = {
	.get_read_latency	= stdev_get_read_latency,
	.get_write_latency	= stdev_get_write_latency,
	.get_stat		= stdev_get_stat,
	.read_block		= stdev_read_block,
	.write_block		= stdev_ssd_write_block,
};


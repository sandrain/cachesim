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
#ifndef	__DEVICE_H__
#define	__DEVICE_H__

#include <linux/types.h>

struct stdev;	/* STorage DEVice */

struct stdev_stat {
	__u64	read_count;
	__u64	write_count;
	__u64	read_blocks;
	__u64	written_blocks;
};

struct stdev_ops {
	double (*get_read_latency) (struct stdev *self);
	double (*get_write_latency) (struct stdev *self);
	int (*get_stat) (struct stdev *self, struct stdev_stat *stat);

	int (*read_block) (struct stdev *self,
				__u64 offset, __u64 count, double *latency);
	int (*write_block) (struct stdev *self,
				__u64 offset, __u64 count, double *latency);
};

struct stdev {
	int	read_latency;
	int	write_latency;
	__u32	block_size;
	__u64	block_count;
	void	*private;

	struct stdev_ops *ops;
	struct stdev_stat stat;
};

struct stdev *stdev_init(__u32 read_latency, __u32 write_latency,
			__u32 block_size, __u64 block_count,
			struct stdev_ops *ops, void *private);

void stdev_exit(struct stdev *self);

static inline double stdev_get_read_latency(struct stdev *self)
{
	if (!self)
		return -1;

	return self->read_latency;
}

static inline double stdev_get_write_latency(struct stdev *self)
{
	if (!self)
		return -1;

	return self->write_latency;
}

static inline int stdev_get_stat(struct stdev *self, struct stdev_stat *stat)
{
	if (!self || !stat)
		return -1;

	*stat = self->stat;
	return 0;
}

static inline int stdev_read_block(struct stdev *self,
				__u64 offset, __u64 count, double *latency)
{
	if (!self)
		return -1;
	if (offset + count > self->block_count)
		return -2;

	self->stat.read_count++;
	self->stat.read_blocks += count;
	*latency = self->read_latency;

	return 0;
}

static inline int stdev_write_block(struct stdev *self,
				__u64 offset, __u64 count, double *latency)
{
	if (!self)
		return -1;
	if (offset + count > self->block_count)
		return -2;

	self->stat.write_count++;
	self->stat.written_blocks += count;
	*latency = self->write_latency;

	return 0;
}

struct stdev_ops generic_stdev_ops;

struct stdev *ssd_init(__u32 read_latency, __u32 write_latency,
			__u32 block_size, __u64 block_count,
			struct stdev_ops *ops, void *private);

struct stdev_ops ssd_dev_ops;

#endif	/* __DEVICE_H__ */


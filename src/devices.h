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
#ifndef	__DEVICES_H__
#define	__DEVICES_H__

#include <linux/types.h>

struct dev_storage {
	__u32	read_latency;
	__u32	write_latency;
	__u64	capacity;
	__u32	blksize;

	__u64	read_count;
	__u64	write_count;

	__u32 (*read_block)(struct dev_storage *self, __u64 block);
	__u32 (*write_block)(struct dev_storage *self, __u64 block);
};

/* TODO:
 * At some point, some devices would need an extension. For example, we need to
 * track the write distribution in ssds.
 */

typedef	struct dev_storage	dev_memory_t;
typedef	struct dev_storage	dev_ssd_t;
typedef	struct dev_storage	dev_hdd_t;

static inline __u32 read_block(struct dev_storage *self, __u64 block)
{
	self->read_count++;
	return self->read_latency;
}

static inline __u32 write_block(struct dev_storage *self, __u64 block)
{
	self->write_count++;
	return self->write_latency;
}

#endif	/* __DEVICES_H__ */


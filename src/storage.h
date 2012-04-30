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
#ifndef	__STORAGE_H__
#define	__STORAGE_H__

#include <linux/types.h>

#include "device.h"
#include "cache.h"

/* Just a wrapper class of cache and stdev. */

struct storage {

#define	STORAGE_TYPE_CACHE	0
#define	STORAGE_TYPE_STABLE	1
#define	N_STORAGE_TEPE		2

	int	type;
	union {
		struct cache *cache;
		struct stdev *stable;
	};
};

static inline int storage_read_block(struct storage *self,
				__u32 req_node, __u64 offset, __u64 count,
				__u64 *nhits, double *latency)
{
	if (!self)
		return -1;

	switch (self->type) {
	case STORAGE_TYPE_CACHE:
		return storage->cache->ops->read_block(storage->cache,
				req_node, offset, count, nhits, latency);
	case STORAGE_TYPE_STABLE:
		*nhits = 0;
		return storage->stable->ops->read_block(storage->stable,
				req_node, offset, count, latency);
	default:
		return -1;
	}
}

static inline int storage_write_block(struct storage *self,
				__u32 req_node, __u64 offset, __u64 count,
				__u64 *nhits, double *latency)
{
	if (!self)
		return -1;

	switch (self->type) {
	case STORAGE_TYPE_CACHE:
		return storage->cache->ops->write_block(storage->cache,
				req_node, offset, count, nhits, latency);
	case STORAGE_TYPE_STABLE:
		*nhits = 0;
		return storage->stable->ops->write_block(storage->stable,
				req_node, offset, count, latency);
	default:
		return -1;
	}
}

#endif	/* __STORAGE_H__ */


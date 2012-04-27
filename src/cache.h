/* Copyright (C) Hyogi Sim <hyogi@cs.vt.edu>
 * 
 * --------------------------------------------------------------------------
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
#ifndef	__CACHE_H__
#define	__CACHE_H__

#include <linux/types.h>

struct stdev;
struct cache;

struct cache_stat {
	__u64	hit_count;
	__u64	miss_count;
};

struct cache_ops {
	int (*get_stat) (struct cache *self, struct cache_stat *stat);
	int (*read_block) (struct cache *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency);
	int (*write_block) (struct cache *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency);
};

struct cache {
	struct stdev **devs;
	struct cache_stat stat;
	struct cache_ops *ops;

	void *private;
};

#endif	/* __CACHE_H__ */


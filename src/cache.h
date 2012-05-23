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
#ifndef	__CACHE_H__
#define	__CACHE_H__

#include <stdio.h>
#include "cachesim.h"

struct local_cache_ops;

enum {
	CACHE_HIT = 0,
	CACHE_MISS
};

enum {
	CACHE_DEV_RAM = 0,
	CACHE_DEV_SSD,
	CACHE_DEV_HDD,
	N_CACHE_DEVS
};

struct local_cache {
	__u32 node;

	struct node *local;
	struct node *pfs;

	__u64 stat_hits;
	__u64 stat_misses;
	__u64 stat_replacements;

	int policy;
	void *private;
	struct local_cache_ops *ops;
};

struct local_cache *local_cache_init(struct local_cache *self, int policy,
				struct node *local, struct node *pfs);

void local_cache_exit(struct local_cache *self);

int local_cache_rw_block(struct local_cache *self, struct io_request *req);

static inline
int local_cache_sync_block(struct local_cache *self, struct io_request *req)
{
	if (self->pfs)	/** send request to the pfs */
		return node_pfs_rw_block(self->pfs, req);
	else		/** send request to local hdd */
		return storage_rw_block(self->local->hdd, req);
}

void local_cache_dump(struct local_cache *self, FILE *fp);

#define	local_cache_fetch_block		local_cache_sync_block

/**
 * Each cache algorithm should implement the following methods.
 * @init is called at the final step of the local_cache_init().
 * @exit is called at the first step of the local_cache_exit().
 * @read_block and @write_block are called inside the local_cache_rw_block
 * function.
 * @dump is called after finishing the simulation. this function should emit
 * the final state of the cache to the given stream @fp.
 */
struct local_cache_ops {
	int (*init) (struct local_cache *self);
	void (*exit) (struct local_cache *self);
	int (*read_block) (struct local_cache *self, struct io_request *req);
	int (*write_block) (struct local_cache *self, struct io_request *req);
	void (*dump) (struct local_cache *self, FILE *fp);
};

enum {
	CACHE_POLICY_NONE = 0,
	CACHE_POLICY_RANDOM,
	CACHE_POLICY_FIFO,
	CACHE_POLICY_LRU,
	CACHE_POLICY_MRU,
	N_CACHE_POLICIES
};

extern struct local_cache_ops none_cache_ops;
extern struct local_cache_ops random_cache_ops;
extern struct local_cache_ops fifo_cache_ops;
extern struct local_cache_ops lru_cache_ops;
extern struct local_cache_ops mru_cache_ops;

#endif	/** __CACHE_H__ */


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

/**
 * local_cache structure represents the cache instance. Each algorithm should
 * implement its own local_cache_ops.
 */
struct local_cache {
	__u32 node;			/* the node where this cache lives */

	struct node *local;		/* local node */
	struct node *pfs;		/* pfs node */

	__u64 stat_hits;		/* statistics information */
	__u64 stat_misses;
	__u64 stat_replacements;

	int policy;			/* cache policy */
	void *private;			/* free hook for cache algorithms */
	struct local_cache_ops *ops;	/* algorithm-specific cache operation
					   table */
};

/**
 * local_cache_init initializes local_cache instance. NOTE that the memory
 * space should be allocated by the caller. At the final phase this function
 * internally calls the algorithm specific init() method.
 *
 * @self: the memory space allocated by the caller.
 * @policy: cache policy to be used.
 * @local: node instance where this cache lives.
 * @pfs: pfs node instance.
 *
 * returns initialized local_cache instance on success. NULL otherwise.
 */
struct local_cache *local_cache_init(struct local_cache *self, int policy,
				struct node *local, struct node *pfs);

/**
 * local_cache_exit destoys the given local_cache instance. This doesn't mean
 * that the memory space of local_cache is freed. That space should be managed
 * by the caller. This function does some internal cleaning works. This
 * internally calls the algorithm specific exit() method.
 *
 * @self: the local_cache instance.
 */
void local_cache_exit(struct local_cache *self);

/**
 * local_cahe_rw_blocks, based on the type of the request @req, internally
 * calls algorithm-specific read_block() or write_block() method.
 *
 * @self: the local_cache instance.
 * @req: the io request.
 *
 * returns number of misses in the cache layer, if algorithms specific methods
 * are correctly implemented.
 */
int local_cache_rw_block(struct local_cache *self, struct io_request *req);

/**
 * local_cache_sync_block syncs the requested block to the stable storage. In
 * case of computing nodes the request goes to the pfs node. And for pfs node,
 * it goes to the local hdd.
 *
 * @self: local_cache structure.
 * @req: io request.
 *
 * returns 0 on success
 */
int local_cache_sync_block(struct local_cache *self, struct io_request *req);

/** This macro just defines an another name of local_cache_sync_block. */
#define	local_cache_fetch_block		local_cache_sync_block

/**
 * local_cache_dump dumps the state of the local_cache instance. This
 * internally calls the dump() method in operation table.
 *
 * @self: local_cache instance.
 * @fp: output stream.
 */
void local_cache_dump(struct local_cache *self, FILE *fp);

/**
 * Each cache algorithm should implement the following methods.
 *
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

/**
 * Newly implemented algorithms should be added here.
 */
enum {
	CACHE_POLICY_NONE = 0,
	CACHE_POLICY_RANDOM,
	CACHE_POLICY_OPT,
	CACHE_POLICY_FIFO,
	CACHE_POLICY_LRU,
	CACHE_POLICY_MRU,
	CACHE_POLICY_LFU,
	CACHE_POLICY_ARC,
#if 0
	CACHE_POLICY_LIRS,
#endif
	N_CACHE_POLICIES
};

extern struct local_cache_ops none_cache_ops;
extern struct local_cache_ops random_cache_ops;
extern struct local_cache_ops opt_cache_ops;
extern struct local_cache_ops fifo_cache_ops;
extern struct local_cache_ops lru_cache_ops;
extern struct local_cache_ops mru_cache_ops;
extern struct local_cache_ops lfu_cache_ops;
extern struct local_cache_ops arc_cache_ops;
#if 0
extern struct local_cache_ops lirs_cache_ops;
extern struct local_cache_ops lrfu_cache_ops;
#endif

#endif	/** __CACHE_H__ */


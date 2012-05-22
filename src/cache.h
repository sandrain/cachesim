#ifndef	__CACHE_H__
#define	__CACHE_H__

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

/**
 * Each cache algorithm should implement the following methods.
 * @init is called at the final step of the local_cache_init().
 * @exit is called at the first step of the local_cache_exit().
 * @read_block and @write_block are called inside the local_cache_rw_block
 * function.
 */
struct local_cache_ops {
	int (*init) (struct local_cache *self);
	void (*exit) (struct local_cache *self);
	int (*read_block) (struct local_cache *self, struct io_request *req);
	int (*write_block) (struct local_cache *self, struct io_request *req);
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


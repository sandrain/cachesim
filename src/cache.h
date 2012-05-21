#ifndef	__CACHE_H__
#define	__CACHE_H__

#include "cachesim.h"

struct local_cache;

struct local_cache_ops {
	int (*read_block) (struct local_cache *self, struct io_request *req);
	int (*write_block) (struct local_cache *self, struct io_request *req);
	int (*insert_block) (struct local_cache *self, struct io_request *req);
};

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
	__u64 block_size;
	struct storage *devs[N_CACHE_DEVS];

	__u64 stat_hits;
	__u64 stat_misses;
	__u64 stat_replacements;

	int policy;
	void *private;
	struct local_cache_ops *ops;
};

struct local_cache *local_cache_init(__u32 node, int policy, int ndevs,
				struct storage *devs[N_CACHE_DEVS],
				struct local_cache_ops *ops, void *data);

void local_cache_exit(struct local_cache *self);

int local_cache_rw_block(struct local_cache *self, struct io_request *req);

int local_cache_insert_block(struct local_cache *self, struct io_request *req);

enum {
	CACHE_POLICY_RANDOM = 0,
	CACHE_POLICY_FIFO,
	CACHE_POLICY_LRU,
	CACHE_POLICY_MRU,
	N_CACHE_POLICIES
};

extern struct local_cache_ops random_cache_ops;
extern struct local_cache_ops fifo_cache_ops;
extern struct local_cache_ops lru_cache_ops;
extern struct local_cache_ops mru_cache_ops;

struct local_cache *local_cache_init_random(__u32 node, int ndevs,
				struct storage *devs[N_CACHE_DEVS]);

struct local_cache *local_cache_init_fifo(__u32 node, int ndevs,
				struct storage *devs[N_CACHE_DEVS]);

struct local_cache *local_cache_init_lru(__u32 node, int ndevs,
				struct storage *devs[N_CACHE_DEVS]);

struct local_cache *local_cache_init_mru(__u32 node, int ndevs,
				struct storage *devs[N_CACHE_DEVS]);

#endif	/** __CACHE_H__ */


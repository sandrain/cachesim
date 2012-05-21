#ifndef	__NODE_H__
#define	__NODE_H__

#include "cachesim.h"

#define	PFS_NODE_ID	0

struct node_operations {
};

struct node {
	__u32 id;

	struct ioapp *app;
	struct local_cache *cache;

	struct storage *ram;
	struct storage *ssd;
	struct storage *hdd;

	struct node *pfs;

	void *private;

	struct node_operations *ops;
};

extern struct node_operations compute_node_operations;
extern struct node_operations pfs_node_operations;

struct node *node_init(struct node *self,
		__u32 id, struct ioapp *app, struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd,
		struct node_operations *ops, void *private);

static inline void node_exit(struct node *self) {}

static inline
struct node *node_init_compute(struct node *self,
		__u32 id, struct ioapp *app, struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd)
{
	return node_init(self, id, app, cache, ram, ssd, hdd,
			&compute_node_operations, NULL);
}

static inline
struct node *node_init_pfs(struct node *self,
		struct storage *ram, struct storage *ssd,
		struct storage *hdd, struct local_cache *cache)
{
	return node_init(self, 0, NULL, cache, ram, ssd, hdd,
			&pfs_node_operations, NULL);
}

static inline
void node_set_pfs(struct node *self, struct node *pfs)
{
	if (self)
		self->pfs = pfs;
}

int node_service_ioapp(struct node *self);

int node_pfs_rw_block(struct node *pfs, struct io_request *req);

void node_get_statistics(struct node *self, struct node_statistics *stat);

#endif	/** __NODE_H__ */


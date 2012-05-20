#ifndef	__NODE_H__
#define	__NODE_H__

#include <linux/types.h>

struct local_cache;
struct node;
struct node_statistics;

/** TODO
 * node_operation represent the following cases:
 *  - The pfs node exposes this to other computes nodes; a compute node can
 *  request a block to the pfs node on local cache miss.
 *  - In a global cache, a compute node can request a block to another compute
 *  node; each compute node should expose this.
 *  - The global cache manager can query to a compute node about the block
 *  existence.
 */
struct node_operations {
	int (*find_block) (struct node *self, __u32 remote, __u64 block);
	int (*read_block) (struct node *self, __u32 remote, __u64 block);
	int (*write_block) (struct node *self, __u32 remote, __u64 block);
};

#define	PFS_NODE_ID	0

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

struct node *node_init(__u32 id, struct ioapp *app, struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd,
		struct node_operations *ops, void *private);

void node_exit(struct node *self);

static inline
struct node *node_init_compute(__u32 id, struct ioapp *app,
		struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd)
{
	return node_init(id, app, cache, ram, ssd, hdd,
			&compute_node_operations, NULL);
}

static inline
struct node *node_init_pfs(struct storage *ram, struct storage *ssd,
		struct local_cache *cache, struct storage *hdd)
{
	return node_init(0, NULL, cache, ram, ssd, hdd, &pfs_node_operations,
			NULL);
}

static inline
void node_set_pfs(struct node *self, struct node *pfs)
{
	if (self)
		self->pfs = pfs;
}

int node_service_ioapp(struct node *self);

void node_get_statistics(struct node *self, struct node_statistics *stat);

#endif	/** __NODE_H__ */


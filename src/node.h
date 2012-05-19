#ifdef	__NODE_H__
#define	__NODE_H__

#include <linux/types.h>

struct local_cache;
struct node;

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

	void *private;

	struct node_opearations *ops;
};

struct node *node_init(__u32 id, struct ioapp *app, struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd,
		struct node_operations *ops);

struct node *node_init_compute(__u32 id, struct ioapp *app,
		struct storage *ram, struct storage *ssd, struct storage *hdd);

struct node *node_init_pfs(struct storage *ram, struct storage *ssd,
			struct storage *hdd);

void node_exit(struct node *self);

extern struct node_operations compute_node_operations;
extern struct node_operations pfs_node_operations;

#endif	/** __NODE_H__ */


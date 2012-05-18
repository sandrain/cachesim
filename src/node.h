#ifdef	__NODE_H__
#define	__NODE_H__

#include <linux/types.h>

struct node_operations;
struct local_cache;

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

struct node_init(__u64 id);

void node_exit(struct node *self);

#endif	/** __NODE_H__ */


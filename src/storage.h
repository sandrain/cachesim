#ifndef	__STORAGE_H__
#define	__STORAGE_H__

#include "cachesim.h"

struct storage;
struct io_request;

struct storage_operations {
	int (*read_block) (struct storage *self, __u64 offset, __u64 len);
	int (*write_block) (struct storage *self, __u64 offset, __u64 len);
};

extern struct storage_operations generic_storage_ops;

struct storage {
	__u32 node;

	__u64 capacity;
	__u64 block_size;
	__u64 block_count;

	__u32 latency_read;
	__u32 latency_write;

	__u64 stat_reads;
	__u64 stat_writes;

	struct storage_operations *ops;

	__u64 *wear;
};

struct storage *storage_init(__u32 node,
			__u64 block_size, __u64 block_count,
			__u32 latency_read, __u32 latency_write,
			int wear,
			struct storage_operations *ops);

static inline struct storage *storage_init_ram(__u32 node,
				__u64 block_size, __u64 block_count)
{
	__u64 latency_read = cachesim_config->ram_latency_read;
	__u64 latency_write = cachesim_config->ram_latency_write;
	int wear = cachesim_config->ram_wear;

	return storage_init(node, block_size, block_count,
				latency_read, latency_write, wear,
				&generic_storage_ops);
}

static inline struct storage *storage_init_ssd(__u32 node,
				__u64 block_size, __u64 block_count)
{
	__u64 latency_read = cachesim_config->ssd_latency_read;
	__u64 latency_write = cachesim_config->ssd_latency_write;
	int wear = cachesim_config->ssd_wear;

	return storage_init(node, block_size, block_count,
				latency_read, latency_write, wear,
				&generic_storage_ops);
}

static inline struct storage *storage_init_hdd(__u32 node,
				__u64 block_size, __u64 block_count)
{
	__u64 latency_read = cachesim_config->hdd_latency_read;
	__u64 latency_write = cachesim_config->hdd_latency_write;
	int wear = cachesim_config->hdd_wear;

	return storage_init(node, block_size, block_count,
				latency_read, latency_write, wear,
				&generic_storage_ops);
}

int storage_rw_block(struct storage *self, struct io_request *req);

void storage_exit(struct storage *self);

#endif	/** __STORAGE_H__ */


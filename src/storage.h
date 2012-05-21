#ifndef	__DEVICE_H__
#define	__DEVICE_H__

#include "cachesim.h"

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

static inline
int storage_rw_block(struct storage *self, struct io_request *req)
{
	switch (req->type) {
	case IOREQ_TYPE_ANY:
	case IOREQ_TYPE_READ:
		self->ops->read_block(self, req->offset, req->len);
		break;
	case IOREQ_TYPE_WRITE:
		self->ops->write_block(self, req->offset, req->len);
		break;
	default:
		break;
	}
}

void storage_exit(struct storage *self);

#endif	/** __DEVICE_H__ */


#ifndef	__DEVICE_H__
#define	__DEVICE_H__

#include <linux/types.h>

struct storage_operations {
	__u64 (*read_block) (__u64 offset, __u64 count);
	__u64 (*write_block) (__u64 offset, __u64 count);
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

	__u64 wear[0];
};

struct storage *storage_init(__u32 node,
			__u64 block_size, __u64 block_count,
			__u32 latency_read, __u32 latency_write,
			int wear,
			struct storage_operations *ops);

struct storage *storage_init_ram(__u32 node,
			__u64 block_size, __u64 block_count);

struct storage *storage_init_ssd(__u32 node,
			__u64 block_size, __u64 block_count);

struct storage *storage_init_hdd(__u32 node,
			__u64 block_size, __u64 block_count);

void storage_exit(struct storage *self);

#endif	/** __DEVICE_H__ */


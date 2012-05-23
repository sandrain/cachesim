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
#ifndef	__STORAGE_H__
#define	__STORAGE_H__

#include "cachesim.h"

struct storage;
struct io_request;

/**
 * storage_operations contains methods of storage instance. If your device
 * needs its own io methods you will define this.
 * @read_block and @write_block takes the following argument.
 *
 * @self: storage instance.
 * @offset: starting block number.
 * @len: number of blocks.
 *
 * They should return 0 on success and <0 on errors.
 */
struct storage_operations {
	int (*read_block) (struct storage *self, __u64 offset, __u64 len);
	int (*write_block) (struct storage *self, __u64 offset, __u64 len);
};

/**
 * Use this for the general block devices.
 */
extern struct storage_operations generic_storage_ops;

/**
 * storage abstracts a general block device and ram as well. The block size is
 * globally define and can be fetched by cachesim_config->block_size.
 */
struct storage {
	__u32 node;			/* node id to which this belongs */

	__u64 block_count;		/* number of blocks: capacity */

	__u32 latency_read;		/* read latency */
	__u32 latency_write;		/* write latency */

	__u64 stat_reads;		/* statistics */
	__u64 stat_writes;

	struct storage_operations *ops; /* Operation table */

	__u64 *wear;
};

/**
 * storage_init initializes a new storage instance. It is prefered to use
 * device specific initialization methods listed below.
 *
 * @self: should be allocated by the caller.
 * @node: node id of this device.
 * @block_count: number of available blocks.
 * @latency_read: read latency.
 * @latency_write: write latency.
 * @wear: do we need to keep track of wear information for this device?
 * @ops: operation table.
 *
 * returns a new storage instance, NULL on errors.
 */
struct storage *storage_init(struct storage *self, __u32 node, __u64
			block_count, __u32 latency_read, __u32 latency_write,
			int wear, struct storage_operations *ops);

/**
 * storage_init_ram is a wrapper for ram device. Since we simulate cache
 * systems ram is also regarded as a block device.
 *
 * @self: should be allocated by the caller.
 * @node: node id.
 * @block_count: number of blocks available.
 *
 * returns a new storage instance, NULL on errors.
 */
static inline struct storage *storage_init_ram(struct storage *self, __u32 node,
				__u64 block_count)
{
	__u64 latency_read = cachesim_config->ram_latency_read;
	__u64 latency_write = cachesim_config->ram_latency_write;
	int wear = cachesim_config->ram_wear;

	return storage_init(self, node, block_count,
				latency_read, latency_write, wear,
				&generic_storage_ops);
}

/**
 * storage_init_ssd, a wrapper for ssd device.
 *
 * @self: should be allocated by the caller.
 * @node: node id.
 * @block_count: number of blocks available.
 *
 * returns a new storage instance, NULL on errors.
 */
static inline struct storage *storage_init_ssd(struct storage *self, __u32 node,
				__u64 block_count)
{
	__u64 latency_read = cachesim_config->ssd_latency_read;
	__u64 latency_write = cachesim_config->ssd_latency_write;
	int wear = cachesim_config->ssd_wear;

	return storage_init(self, node, block_count,
				latency_read, latency_write, wear,
				&generic_storage_ops);
}

/**
 * storage_init_ssd, a wrapper for ssd device.
 *
 * @self: should be allocated by the caller.
 * @node: node id.
 * @block_count: number of blocks available.
 *
 * returns a new storage instance, NULL on errors.
 */
static inline struct storage *storage_init_hdd(struct storage *self, __u32 node,
				__u64 block_count)
{
	__u64 latency_read = cachesim_config->hdd_latency_read;
	__u64 latency_write = cachesim_config->hdd_latency_write;
	int wear = cachesim_config->hdd_wear;

	return storage_init(self, node, block_count,
				latency_read, latency_write, wear,
				&generic_storage_ops);
}

/**
 * storage_exit de-initializes the storage instance.
 *
 * @self: storage instance.
 */
void storage_exit(struct storage *self);

/**
 * storage_rw_block is invoked by the external modules to request a block io
 * operation to a storage device. This internally calls @read_block or
 * @write_block method in @self->ops.
 *
 * @self: storage instance.
 * @req: block io request.
 *
 * returns 0 on success, <0 on errors.
 */
int storage_rw_block(struct storage *self, struct io_request *req);

#endif	/** __STORAGE_H__ */


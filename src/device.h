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
#ifndef	__DEVICE_H__
#define	__DEVICE_H__

#include <linux/types.h>

struct stdev;	/* STorage DEVice */

struct stdev_stat {
	__u64	read_count;
	__u64	write_count;
	__u64	read_blocks;
	__u64	written_blocks;
};

struct stdev_ops {
	double (*get_read_latency) (struct stdev *self);
	double (*get_write_latency) (struct stdev *self);
	int (*get_stat) (struct stdev *self, struct stdev_stat *stat);

	int (*read_block) (struct stdev *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency);
	int (*write_block) (struct stdev *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency);
};

struct stdev {

#define	STDEV_MEM	0
#define	STDEV_HDD	1
#define	STDEV_SSD	2
#define	N_STDEVTYPE	3

	int	type;
	__u32	node;
	int	read_latency;
	int	write_latency;
	__u32	block_size;
	__u64	block_count;
	void	*private;

	/* FIXME: It's not a good idea that every device holds this. */
	struct network *network;

	struct stdev_ops *ops;
	struct stdev_stat stat;
};

/**
 * @brief	Allocate/Initialize an instance. The instance should be freed
 *		by calling stdev_exit()
 *
 * @type	Type of this device
 * @read_latency The latency to read one block
 * @write_latency The latency to write one block
 * @block_size	Block size
 * @block_count Total number of blocks
 * @ops		Operation table
 * @private	Private data to keep
 *
 * @return	Newly allocated/initialized instance. NULL on failure.
 */
struct stdev *stdev_init(int type,
			__u32 read_latency, __u32 write_latency,
			__u32 block_size, __u64 block_count,
			struct stdev_ops *ops, void *private);

/**
 * @brief	Free the stdev instance
 *
 * @self	Pointer to the instance
 */
void stdev_exit(struct stdev *self);

/**
 * @brief	Set node number for device instance
 *
 * @self	Instance
 * @node	Node number
 *
 * @return	0 on success, -1 for invalid instance.
 */
static inline int stdev_set_node(struct stdev *self, __u32 node)
{
	if (!self)
		return -1;

	self->node = node;
	return 0;
}

/**
 * @brief	Get the lantecy for reading a block
 *
 * @self	Instance
 *
 * @return	The latency on success, -1 on invalid instance.
 */
double stdev_get_read_latency(struct stdev *self);

/**
 * @brief	Get the latency for writing a block
 *
 * @self	Instance
 *
 * @return	The latency on success, -1 on invalid instance.
 */
double stdev_get_write_latency(struct stdev *self);

/**
 * @brief	Get statistics up to a certain point.
 *
 * @self	Instance
 * @stat	[out] Space to be filled with this function; caller should
 *		allocate this.
 *
 * @return	0 on success, -1 on invalid instance.
 */
int stdev_get_stat(struct stdev *self, struct stdev_stat *stat);

/**
 * @brief	Simulate reading blocks.
 *
 * @self	Instance
 * @req_node	Caller's node number (to estimate network delay)
 * @offset	Starting block number
 * @count	Number of blocks to be read
 * @latency	[out] Expected latency, filled with this function
 *
 * @return	0 on success, -1 on invalid instance.
 */
int stdev_read_block(struct stdev *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency);

/**
 * @brief	Simulate writing blocks.
 *
 * @self	Instance
 * @req_node	Caller's node number (to estimate network delay)
 * @offset	Starting block number
 * @count	Number of block to be written
 * @latency	[out] Expected latency, filled with this function
 *
 * @return	0 on success, -1 on invalid instance.
 */
int stdev_write_block(struct stdev *self, __u32 req_node,
				__u64 offset, __u64 count, double *latency);

struct stdev_ssd_stat {
	__u32	flash_block_size;
	__u64	write_dist[0];
};

/**
 * @brief	Initialization routine for ssd devices. This is derived for
 *		accounting write distribution across the flash device.
 *
 * @read_latency Latency for reading a block
 * @write_latency Latency for writing a block
 * @block_size	IO block size
 * @block_count	Total number of blocks
 * @flash_block_size	Physical block size of NAND Flash memory.
 *
 * @return	Newly created instance, which should be freed with stdev_exit.
 */
struct stdev *ssd_init(__u32 read_latency, __u32 write_latency,
			__u32 block_size, __u64 block_count,
			__u32 flash_block_size);

/**
 * wrappers of stdev_init()
 */
#define	mem_init(read_latency, write_latency, block_size, block_count)	\
		stdev_init(read_latency, write_latency,			\
			block_size, block_count, &generic_stdev_ops, NULL)

#define	hdd_init(read_latency, write_latency, block_size, block_count)	\
		stdev_init(read_latency, write_latency,			\
			block_size, block_count, &generic_stdev_ops, NULL)

struct stdev_ops generic_stdev_ops;
struct stdev_ops ssd_dev_ops;

#endif	/* __DEVICE_H__ */


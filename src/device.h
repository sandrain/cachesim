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

struct storage_dev;

struct storage_dev_stat {
	__u64	read_count;
	__u64	write_count;
	__u64	read_blocks;
	__u64	written_blocks;
};

struct storage_dev_ops {
	struct storage_dev *(*init) (__u32 read_latency, __u32 write_latency,
				__u32 block_size, __u64 block_count,
				struct storage_dev_ops *ops, void *private);
	void (*exit) (struct storage_dev *stdev);

	double (*get_read_latency) (struct storage_dev *stdev);
	double (*get_write_latency) (struct storage_dev *stdev);
	int (*get_stat) (struct storage_dev *stdev,
			struct storage_dev_stat *stat);

	int (*read_block) (struct storage_dev *stdev,
				__u64 offset, __u64 count, double *latency);
	int (*write_block) (struct storage_dev *stdev,
				__u64 offset, __u64 count, double *latency);
};

struct storage_dev {
	int	read_latency;
	int	write_latency;
	__u32	block_size;
	__u64	block_count;
	void	*private;

	struct storage_dev_ops *ops;
	struct storage_dev_stat stat;
};

/**
 * @brief
 *
 * @read_latency
 * @write_latency
 * @block_size
 * @block_count
 * @ops
 * @private
 *
 * @return 
 */
struct storage_dev *storage_dev_init(__u32 read_latency, __u32 write_latency,
				__u32 block_size, __u64 block_count,
				struct storage_dev_ops *ops, void *private);

/**
 * @brief
 *
 * @stdev
 */
void storage_dev_exit(struct storage_dev *stdev);

/**
 * @brief
 *
 * @stdev
 *
 * @return 
 */
static inline double generic_get_read_latency(struct storage_dev *stdev)
{
	if (!stdev)
		return -1;

	return stdev->read_latency;
}

/**
 * @brief
 *
 * @stdev
 *
 * @return 
 */
static inline double generic_get_write_latency(struct storage_dev *stdev)
{
	if (!stdev)
		return -1;

	return stdev->write_latency;
}

/**
 * @brief
 *
 * @stdev
 * @stat
 *
 * @return 
 */
static inline int generic_get_stat(struct storage_dev *stdev,
				struct storage_dev_stat *stat)
{
	if (!stdev || !stat)
		return -1;

	*stat = stdev->stat;
	return 0;
}

/**
 * @brief
 *
 * @stdev
 * @offset
 * @count
 * @latency
 *
 * @return 
 */
static inline int generic_read_block(struct storage_dev *stdev,
			__u64 offset, __u64 count, double *latency)
{
	if (!stdev)
		return -1;

	stdev->stat.read_count++;
	stdev->stat.read_blocks += count;
	*latency = stdev->read_latency;

	return 0;
}

/**
 * @brief
 *
 * @stdev
 * @offset
 * @count
 * @latency
 *
 * @return 
 */
static inline int generic_write_block(struct storage_dev *stdev,
			__u64 offset, __u64 count, double *latency)
{
	if (!stdev)
		return -1;

	stdev->stat.write_count++;
	stdev->stat.written_blocks += count;
	*latency = stdev->write_latency;

	return 0;
}

struct storage_dev_ops generic_storage_dev_ops;

#endif	/* __DEVICE_H__ */


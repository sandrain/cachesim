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
#include <stdio.h>
#include <stdlib.h>

#include "cachesim.h"
#include "cache_util.h"

struct fifo_data {
	__u64 block_count;
	__u64 alloc_pos;
	__u64 seq;
	struct cache_meta block_info[0];
};

static __u64 search_block(struct fifo_data *self, __u64 block)
{
	__u64 pos;
	struct cache_meta *binfo = self->block_info;

	for (pos = 0; pos < self->block_count; pos++) {
		struct cache_meta *current = &binfo[pos];

		if (current->block == block)
			return pos;
	}

	return BLOCK_INVALID;
}

static inline __u64 get_next_position(struct fifo_data *fifo)
{
	__u64 pos = fifo->alloc_pos++;

	if (fifo->alloc_pos == fifo->block_count)
		fifo->alloc_pos = 0;

	return pos;
}

/**
 * get_free_block returns the position of a new available cache entry. when
 * there are free blocks available, returns the first entry among them. if
 * there is no free entry, this replaces existing blocks in fifo order.
 *
 * @self: fifo_data instance
 *
 * returns index of the available cache entry. this function should not fail.
 */
static __u64 get_free_block(struct local_cache *cache)
{
	struct fifo_data *fifo = (struct fifo_data *) cache->private;
	__u64 pos = get_next_position(fifo);
	struct cache_meta *binfo = &fifo->block_info[pos];

	if (binfo->block == BLOCK_INVALID)
		return pos;	/** unused entry. nothing more to do */

	if (binfo->dirty == BLOCK_DIRTY)
		cache_sync_block(cache, binfo->block);

	init_cache_entry(binfo);
	cache->stat_replacements++;

	return pos;
}

static int fifo_init(struct local_cache *cache)
{
	__u64 i;
	struct node *local = cache->local;
	struct fifo_data *data = NULL;
	__u64 block_count = local->ram->block_count;

	data = malloc(sizeof(struct fifo_data)
			+ sizeof(struct cache_meta) * block_count);
	if (!data)
		return -ENOMEM;

	data->block_count = block_count;
	data->alloc_pos = 0;
	data->seq = 0;

	for (i = 0; i < block_count; i++)
		init_cache_entry(&data->block_info[i]);

	cache->private = data;
	return 0;
}

static void fifo_exit(struct local_cache *cache)
{
	if (cache && cache->private)
		free(cache->private);
}

static int fifo_rw_block(struct local_cache *cache, struct io_request *req)
{
	int res = 0, type = 0;
	struct fifo_data *fifo = (struct fifo_data *) cache->private;
	struct cache_meta *binfo;
	__u64 i, pos;

	for (i = 0; i < req->len; i++) {
		__u64 current = req->offset + i;
		if ((pos = search_block(fifo, current)) == BLOCK_INVALID) {
			/** cache miss */

			res++;
			pos = get_free_block(cache);

			cache_fetch_block(cache, current);

			binfo = &fifo->block_info[pos];
			binfo->block = current;
			if (req->type == IOREQ_TYPE_WRITE)
				binfo->dirty = BLOCK_DIRTY;

			binfo->seq = fifo->seq++;

			type = IOREQ_TYPE_WRITE;
		}
		else {	/** cache hit */
			cache->stat_hits++;
			if (req->type == IOREQ_TYPE_WRITE)
				fifo->block_info[pos].dirty = BLOCK_DIRTY;

			type = IOREQ_TYPE_READ;
		}

		cache_rw_cache_dev(cache, current, type);
	}

	cache->stat_misses += res;

	return res;
}

static void fifo_dump(struct local_cache *cache, FILE *fp)
{
	struct fifo_data *fifo = (struct fifo_data *) cache->private;

	generic_cache_dump(fifo->block_info, fifo->block_count, fp);
}

struct local_cache_ops fifo_cache_ops = {
	.init		= &fifo_init,
	.exit		= &fifo_exit,
	.read_block	= &fifo_rw_block,
	.write_block	= &fifo_rw_block,
	.dump		= &fifo_dump,
};


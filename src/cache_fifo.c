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

static int fifo_init(struct local_cache *cache)
{
	__u64 i;
	struct node *local = cache->local;
	struct fifo_data *data = NULL;
	__u64 block_count = local->ram->block_count;

	data = malloc(sizeof(struct fifo_data)
			+ sizeof(struct cache_meta) * block_count);
	if (!data)
		return NULL;

	data->block_count = block_count;
	data->alloc_pos = 0;
	data->seq = 0;

	for (i = 0; i < block_count; i++) {
		struct cache_meta *current = &data->block_info[i];
		current->dirty = BLOCK_CLEAN;
		current->block = BLOCK_INVALID;
		current->seq = 0;
		current->private = NULL;
	}

	cache->private = data;
	return 0;
}

static void fifo_exit(struct local_cache *cache)
{
	if (cache && cache->private)
		free(cache->private);
}

static int fifo_rw_block(struct local_cache *cache, struct io_request req)
{
	int res = 0;
	struct fifo_data *fifo = (struct fifo_data *) cache->private;
	struct io_request tmp;
	__u64 i, pos;

	for (i = 0; i < req->len; i++) {
		__u64 current = req->offset + i;
		if ((pos = search_block(fifo, current)) == BLOCK_INVALID) {
			res++;
			pos = next_pos(fifo);
			replace_block(cache, pos, current, dirty);

			tmp.type = IOREQ_TYPE_WRITE;
		}
		else {	/** cache hit */
			cache->stat_hits++;
			fifo->block_info[pos].dirty = dirty;

			tmp.type = IOREQ_TYPE_READ;
		}

		tmp.offset = pos;
		tmp.len = 1;
		storage_rw_block(cache->local->ram, &tmp);
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


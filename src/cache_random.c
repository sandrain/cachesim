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
#include <stdlib.h>
#include <sys/time.h>

#include "cachesim.h"
#include "cache_util.h"

struct rand_data {
	__u64 block_count;
	__u64 seq;
	struct cache_meta block_info[0];
};

static int rand_initialized;

static inline __u64 get_random_frame(__u64 max)
{
	return (__u64) (drand48() * max);
}

static __u64 search_block(struct rand_data *self, __u64 block)
{
	__u64 i;

	for (i = 0; i < self->block_count; i++) {
		struct cache_meta *current = &self->block_info[i];

		if (current->block == BLOCK_INVALID)
			break;

		if (current->block == block)
			return i;
	}

	return BLOCK_INVALID;
}

/**
 * get_free_frame; it first tries to find the empty frame in sequential
 * fashion. after consuming all the free frames it randomly choose a victim.
 *
 * @self: the random_data instance.
 *
 * returns the index of the free frame.
 *
 * NOTE: this function increases the self->seq by 1; no other functions should
 * touch it.
 */
static inline __u64 get_free_frame(struct rand_data *self)
{
	if (self->seq < self->block_count)
		return self->seq++;
	else
		return get_random_frame(self->block_count);
}

static void replace_block(struct local_cache *cache, __u64 frame, __u64 block,
			int dirty)
{
	struct rand_data *self = (struct rand_data *) cache->private;
	struct cache_meta *binfo = &self->block_info[frame];

	if (binfo->block != BLOCK_INVALID) {
		if (binfo->dirty) {	/** we have to sync the block */
			struct io_request req;

			req.offset = block;
			req.len = 1;
			req.type = dirty ? IOREQ_TYPE_WRITE : IOREQ_TYPE_READ;

			local_cache_sync_block(cache, &req);
		}
	}

	binfo->dirty = dirty;
	binfo->block = block;
}

static int random_init(struct local_cache *cache)
{
	__u64 i, block_count = 0;
	struct node *local = cache->local;
	struct rand_data *data = NULL;

	block_count = local->ram->capacity / cachesim_config->block_size;

	data = malloc(sizeof(struct rand_data) +
			sizeof(struct cache_meta) * block_count);
	if (data == NULL)
		return -ENOMEM;

	data->block_count = block_count;
	data->seq = 0;

	for (i = 0; i < block_count; i++) {
		struct cache_meta *current = &data->block_info[i];
		current->dirty = BLOCK_CLEAN;
		current->block = BLOCK_INVALID;
		current->seq = 0;
		current->private = NULL;
	}

	if (!rand_initialized) {
		struct timeval tv;

		rand_initialized = 1;

		gettimeofday(&tv, NULL);
		srand48(tv.tv_usec);
	}

	cache->private = data;

	return 0;
}

static void random_exit(struct local_cache *cache)
{
	if (cache && cache->private)
		free(cache->private);
}

static int random_rw_block(struct local_cache *cache, struct io_request *req)
{
	__u64 i, current, frame;
	int res = 0;
	int dirty = req->type == IOREQ_TYPE_WRITE ? 1 : 0;
	struct rand_data *self = (struct rand_data *) cache->private;

	for (i = 0; i < req->len; i++) {
		current = req->offset + i;

		if (search_block(self, current) == BLOCK_INVALID) {
			res++;
			frame = get_free_frame(self);
			replace_block(cache, frame, current, dirty);
		}
		else {
			struct io_request tmp;

			cache->stat_hits++;
			self->block_info[i].dirty = dirty;

			tmp.type = req->type;
			tmp.offset = current;
			tmp.len = 1;
			storage_rw_block(cache->local->ram, &tmp);
		}
	}

	cache->stat_misses += res;

	return 0;
}

struct local_cache_ops random_cache_ops = {
	.init		= &random_init,
	.exit		= &random_exit,
	.read_block	= &random_rw_block,
	.write_block	= &random_rw_block,
};


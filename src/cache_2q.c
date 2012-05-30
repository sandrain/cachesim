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

#define	_DEBUG_2Q

/** Kin, Kout params; according to the original paper, the reasonable
 * parameters are Kin = 0.3c and Kout = 0.5c where c denotes the cache
 * capacity.
 */
#define	KIN		30
#define	KOUT		50

struct twoq_data {
	__u64 block_count;
	__u64 in_count;
	__u64 out_count;
	__u64 ghost_count;
	__u64 residents;
	__u64 seq;
	__u64 alloc_seq;

	struct cache_meta_list a1in;
	struct cache_meta_list a1out;
	struct cache_meta_list am;

	struct hash_table *htable;
	struct cache_meta block_info[0];
};

static struct cache_meta *alloc_next_block(struct twoq_data *self)
{
	assert(self->alloc_seq <= self->block_count + self->ghost_count);
	return &self->block_info[self->alloc_seq++];
}

static struct cache_meta *search_block(struct twoq_data *self, __u64 block)
{
	return hash_table_search(htable, &block, sizeof(block));
}

static void reclaim(struct twoq_data *self)
{
}

static int twoq_init(struct local_cache *cache)
{
	__u64 i, block_count, total_block_count;
	struct hash_table *htable = NULL;
	struct twoq_data *self = NULL;

	block_count = cache_get_block_count(cache);
	total_block_count = block_count + block_count * KOUT / 100;

	htable = hash_table_init(block_count);
	if (!htable)
		return -errno;

	self = malloc(sizeof(*self) +
			total_block_count * sizeof(struct cache_meta));
	if (!self) {
		hash_table_exit(htable);
		return -errno;
	}

	cache_meta_list_init(&self->a1in);
	cache_meta_list_init(&self->a1out);
	cache_meta_list_init(&self->am);

	self->block_count = block_count;
	self->in_count = block_count * KIN / 100;
	self->out_count = block_count - self->in_count;
	self->ghost_count = block_count * KOUT / 100;
	self->residents = 0;
	self->seq = 0;
	self->alloc_seq = 0;
	self->htable = htable;

	for (i = 0; i < total_block_count; i++) {
		struct cache_meta *current = self->block_info[i];

		init_cache_entry(current);
		current->index = i;
	}

	self->cache = cache;
	cache->private = self;

	return 0;
}

static void twoq_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct twoq_data *self = (struct twoq_data *) cache->self;

		if (self->htable)
			hash_table_exit(self->htable);
		free(self);
	}
}

static void twoq_rw_block(struct local_cache *cache, struct io_request *req)
{
}

static void twoq_dump(struct local_cache *cache)
{
}

struct local_cache_ops twoq_cache_ops = {
	.init		= &twoq_init,
	.exit		= &twoq_exit,
	.read_block	= &twoq_rw_block,
	.write_block	= &twoq_rw_block,
	.dump		= &twoq_dump,
};


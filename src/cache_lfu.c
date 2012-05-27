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

struct lfu_data {
	__u64 block_count;
	__u64 alloc_seq;

	struct local_cache *cache;
	struct hash_table *htable;
	struct cache_meta block_info[0];
};

/** lfu uses cache_meta.@private field to keep the frequency of each block */

static struct cache_meta *get_free_block(struct lfu_data *self)
{
}

static struct cache_meta *search_block(struct lfu_data *self, __u64 block)
{
	struct cache_meta *entry = NULL;
	struct local_cache *cache = self->cache;

	entry = hash_table_search(self->htable, &block, sizeof(block));
	if (entry)
		cache->stat_hits++;
	else
		cache->stat_misses++;

	return entry;
}

static int lfu_init(struct local_cache *cache)
{
	__u64 i;
	__u64 block_count = cache_get_block_count(cache);
	struct lfu_data *self = NULL;
	struct hash_table *htable = NULL;

	htable = hash_table_init(block_count);
	if (!htable)
		return -ENOMEM;

	self = malloc(sizeof(*self) + sizeof(struct cache_meta) * block_count);
	if (!self) {
		hash_table_exit(htable);
		return -ENOMEM;
	}

	for (i = 0; i < block_count; i++) {
		struct cache_meta *current = &self->block_info[i];

		init_cache_entry(current);
		current->index = i;
		current->private = (void *) 0;
	}

	self->block_count = block_count;
	self->cache = cache;
	self->htable = htable;
	cache->private = self;

	return 0;
}

static void lfu_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct lfu_data *self = (struct lfu_data *) cache->private;

		if (self->htable)
			hash_table_exit(self->htable);

		free(self);
	}
}

static int lfu_rw_block(struct local_cache *cache, struct io_request *req)
{
}

static void lfu_dump(struct local_cache *self, FILE *fp)
{
}

struct local_cache_ops lfu_cache_ops = {
	.init		= &lfu_init,
	.exit		= &lfu_exit,
	.read_block	= &lfu_rw_block,
	.write_block	= &lfu_rw_block,
	.dump		= &lfu_dump,
};


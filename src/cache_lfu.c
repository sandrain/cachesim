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

#if 0
#define	_LFU_DEBUG
#endif

struct lfu_data {
	__u64 block_count;
	__u64 alloc_seq;
	__u64 seq;

	struct local_cache *cache;
	struct hash_table *htable;
	struct cache_meta block_info[0];
};

/** lfu uses cache_meta.@private field to keep the frequency of each block */

/** TODO: This function should be re-written by using the priority queue. */
static struct cache_meta *get_lfu_block(struct lfu_data *self)
{
	__u64 i, min = (__u64) -1, seq = (__u64) -1, freq;
	struct cache_meta *current, *lfu = NULL;

	for (i = 0; i < self->block_count; i++) {
		current = &self->block_info[i];
		freq = (__u64) current->private;
		if (freq < min) {
			min = freq;
		}
	}

	for (i = 0; i < self->block_count; i++) {
		current = &self->block_info[i];
		freq = (__u64) current->private;
		if (freq == min) {
			if (current->seq < seq) {
				seq = current->seq;
				lfu = current;
			}
		}
	}

	return lfu;
}

static struct cache_meta *get_free_block(struct lfu_data *self)
{
	__u64 pos;

	if (self->alloc_seq < self->block_count) {
		pos = self->alloc_seq++;
		return &self->block_info[pos];
	}

	return NULL;
}

static void replacement(struct lfu_data *self, __u64 block, int dirty)
{
	struct cache_meta *entry = NULL;

	entry = get_free_block(self);
	if (!entry) {
		entry = get_lfu_block(self);
		if (entry->dirty)
			cache_sync_block(self->cache, entry->block);

		hash_table_delete(self->htable, &entry->block,
				sizeof(entry->block));
		init_cache_entry(entry);

		self->cache->stat_replacements++;
	}

	cache_fetch_block(self->cache, block);
	entry->seq = self->seq;
	entry->dirty = dirty;
	entry->block = block;
	entry->private = (void *) 1;

	hash_table_insert(self->htable, &block, sizeof(block), entry);
}

static inline void increment_frequency(struct cache_meta *entry)
{
	__u64 current = (__u64) entry->private;
	entry->private = (void *) (current + 1);
}

static struct cache_meta *search_block(struct lfu_data *self, __u64 block)
{
	struct cache_meta *entry = NULL;
	struct local_cache *cache = self->cache;

	entry = hash_table_search(self->htable, &block, sizeof(block));
	if (entry) {
		cache->stat_hits++;
		increment_frequency(entry);
	}
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

	self->alloc_seq = 0;
	self->seq = 0;
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

static void lfu_dump(struct local_cache *cache, FILE *fp);

static int lfu_rw_block(struct local_cache *cache, struct io_request *req)
{
	__u64 i, block;
	int res = 0, dirty;
	struct cache_meta *entry = NULL;
	struct lfu_data *self = (struct lfu_data *) cache->private;

	for (i = 0; i < req->len; i++) {
		self->seq++;

		block = req->offset + i;
		dirty = req->type == IOREQ_TYPE_WRITE ? 1 : 0;

		entry = search_block(self, block);
		if (entry) {
			entry->seq = self->seq;
			if (dirty)
				entry->dirty = 1;
		}
		else {
			res++;
			replacement(self, block, dirty);
		}
	}

#ifdef	_LFU_DEBUG
	lfu_dump(cache, cachesim_config->output);
	(void) getchar();
#endif

	return res;
}

/** TODO: this function should be re-written so that it can print entries in
 * lfu order. */
static void lfu_dump(struct local_cache *cache, FILE *fp)
{
	__u64 i;
	struct lfu_data *self = (struct lfu_data *) cache->private;
	struct cache_meta *current;

	for (i = 0; i < self->block_count; i++) {
		current = &self->block_info[i];
		fprintf(fp,	"[%5llu] %d, %llu, freq = %llu, seq = %llu\n",
				i, current->dirty,
				current->block, (__u64) current->private,
				current->seq);
	}
}

struct local_cache_ops lfu_cache_ops = {
	.init		= &lfu_init,
	.exit		= &lfu_exit,
	.read_block	= &lfu_rw_block,
	.write_block	= &lfu_rw_block,
	.dump		= &lfu_dump,
};


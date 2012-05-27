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

static const int LRU = 0;
static const int MRU = 1;

struct lru_data {
	__u64 block_count;
	int policy;	/** LRU or MRU */
	__u64 seq;
	__u64 alloc_seq;

	struct cache_meta_list list;	/* head: mru, tail: lru */

	struct hash_table *htable;

	struct cache_meta block_info[0];
};

static inline void move_to_mru(struct lru_data *self, __u64 pos)
{
	struct cache_meta *entry = &self->block_info[pos];
	struct cache_meta_list *list = &self->list;

	entry = cache_meta_list_remove(list, entry);
	if (entry)
		cache_meta_list_insert_head(list, entry);
}

static inline void add_to_mru(struct lru_data *self, __u64 pos)
{
	struct cache_meta *entry = &self->block_info[pos];

	cache_meta_list_insert_head(&self->list, entry);
}

static inline __u64 get_lru_pos(struct lru_data *self)
{
	unsigned long entry = (unsigned long) self->list.tail;
	unsigned long base = (unsigned long) self->block_info;

	return entry ? (entry - base) / sizeof(struct cache_meta) : 0;
}

static inline __u64 get_mru_pos(struct lru_data *self)
{
	unsigned long entry = (unsigned long) self->list.head;
	unsigned long base = (unsigned long) self->block_info;

	return entry ? (entry - base) / sizeof(struct cache_meta) : 0;
}

static __u64 search_block(struct local_cache *cache, __u64 block)
{
	__u64 pos;
	struct lru_data *self = (struct lru_data *) cache->private;
	struct cache_meta *entry = NULL;

	entry = (struct cache_meta *) hash_table_search(
					self->htable, &block, sizeof(block));
	if (entry) {
		cache->stat_hits++;

		pos = entry->index;
		move_to_mru(self, pos);

		return pos;
	}
#if 0
	struct cache_meta *current;

	for (pos = 0; pos < self->block_count; pos++) {
		current = &self->block_info[pos];

		if (current->block == block) {
			cache->stat_hits++;
			move_to_mru(self, pos);
			return pos;
		}
	}
#endif
	cache->stat_misses++;

	return BLOCK_INVALID;
}

static __u64 get_free_block(struct local_cache *cache)
{
	struct lru_data *self = (struct lru_data *) cache->private;
	struct cache_meta *binfo = NULL;
	__u64 pos;

	if (self->alloc_seq < self->block_count) {
		pos = self->alloc_seq++;
		add_to_mru(self, pos);
		return pos;
	}

	/** we have no free blocks */
	pos = self->policy == LRU ? get_lru_pos(self) : get_mru_pos(self);
	binfo = &self->block_info[pos];

	if (binfo->dirty)
		cache_sync_block(cache, binfo->block);

	hash_table_delete(self->htable, &binfo->block, sizeof(binfo->block));

	move_to_mru(self, pos);
	init_cache_entry_list(binfo);
	cache->stat_replacements++;

	return pos;
}

static struct lru_data *init_self(struct local_cache *cache, int policy)
{
	__u64 i;
	__u64 block_count = cache->local->ram->block_count;
	struct lru_data *self = NULL;
	struct hash_table *htable = NULL;

	htable = hash_table_init(block_count);
	if (!htable)
		return NULL;

	self = malloc(sizeof(struct lru_data)
			+ sizeof(struct cache_meta) * block_count);
	if (self) {
		self->policy = policy;
		self->block_count = block_count;
		self->seq = 0;
		self->alloc_seq = 0;
		self->htable = htable;

		cache_meta_list_init(&self->list);

		for (i = 0; i < block_count; i++) {
			init_cache_entry(&self->block_info[i]);
			self->block_info[i].index = i;
		}
	}

	return self;
}

static int lru_init(struct local_cache *cache)
{
	struct lru_data *self = init_self(cache, LRU);

	if (!self)
		return -ENOMEM;

	cache->private = self;
	return 0;
}

static int mru_init(struct local_cache *cache)
{
	struct lru_data *self = init_self(cache, MRU);

	if (!self)
		return -ENOMEM;

	cache->private = self;
	return 0;
}

static void lru_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct lru_data *self = cache->private;
		if (self->htable)
			hash_table_exit(self->htable);

		free(self);
	}
}

static int lru_rw_block(struct local_cache *cache, struct io_request *req)
{
	int res = 0, type= 0;
	struct lru_data *self = (struct lru_data *) cache->private;
	struct cache_meta *binfo;
	__u64 i, pos;

	for (i = 0; i < req->len; i++) {
		__u64 current = req->offset + i;

		if ((pos = search_block(cache, current)) == BLOCK_INVALID) {
			/** cache miss */
			res++;

			pos = get_free_block(cache);

			cache_fetch_block(cache, current);

			binfo = &self->block_info[pos];
			binfo->block = current;
			if (req->type == IOREQ_TYPE_WRITE)
				binfo->dirty = BLOCK_DIRTY;
			binfo->seq = self->seq++;

			hash_table_insert(self->htable, &current,
					sizeof(__u64), binfo);

			type = IOREQ_TYPE_WRITE;
		}
		else {	/** cache hit */
			if (req->type == IOREQ_TYPE_WRITE)
				self->block_info[pos].dirty = BLOCK_DIRTY;

			type = IOREQ_TYPE_READ;
		}

		cache_rw_cache_dev(cache, pos, type);
	}

	return res;
}

static void lru_dump(struct local_cache *cache, FILE *fp)
{
	__u64 pos = 0;
	struct lru_data *self = (struct lru_data *) cache->private;
	struct cache_meta *binfo = self->list.head;

	fputs("MRU\n", fp);

	while (binfo) {
		generic_cache_entry_dump(pos++, binfo, fp);
		binfo = binfo->next;
	}

	fputs("LRU\n", fp);
}

struct local_cache_ops lru_cache_ops = {
	.init		= &lru_init,
	.exit		= &lru_exit,
	.read_block	= &lru_rw_block,
	.write_block	= &lru_rw_block,
	.dump		= &lru_dump,
};

struct local_cache_ops mru_cache_ops = {
	.init		= &mru_init,
	.exit		= &lru_exit,
	.read_block	= &lru_rw_block,
	.write_block	= &lru_rw_block,
	.dump		= &lru_dump,
};


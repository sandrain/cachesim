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
/**
 * The implementation is based on the algorithm (Fig. 4) in the original ARC
 * paper:
 *  - Nimrod Megiddo and Dharmendra S. Modha
 *    ARC: A Self-Tuning, Low Overhead Replacement Cache, FAST 2003
 *
 * Tried to implemented the algorithm as it is.
 */
#include <stdio.h>
#include <stdlib.h>

#include "cachesim.h"
#include "cache_util.h"

/**
 * arc cache_meta usage (struct cache_meta)
 * @dirty: clean(0) or dirty(1)?
 * @index: the index in block_info[0] array.
 * @block: physical block number.
 * @seq: the list it belongs to.
 */

#define	T1	1
#define	B1	2
#define	T2	3
#define	B2	4

struct arc_data {
	__s64 p;			/* adaptation value */

	struct cache_meta_list t1;	/* L1, recency list */
	struct cache_meta_list b1;
	struct cache_meta_list t2;	/* L2, frequency list */
	struct cache_meta_list b2;

	struct local_cache *cache;

	__u64 block_count;		/* cache size */
	struct cache_meta block_info[0]; /* cache size * 2 (includes ghosts) */
};

static inline
void arc_entry_init(struct cache_meta *entry, __u64 block, int type)
{
	if (entry) {
		entry->dirty = type == IOREQ_TYPE_READ ? 0 : 1;
		entry->block = block;
	}
}

static inline
struct cache_meta_list *get_list(struct arc_data *self, __u64 id)
{
	switch (id) {
	case T1: return &self->t1;
	case B1: return &self->b1;
	case T2: return &self->t2;
	case B2: return &self->b2;
	default: return NULL;
	}
}

static inline
__u64 get_list_size(struct arc_data *self, __u64 id)
{
	struct cache_meta_list *list = get_list(self, id);
	return list->size;
}

static inline void adaptation(struct arc_data *self, __u64 id)
{
	__u64 b1 = self->b1.size;
	__u64 b2 = self->b2.size;
	__s64 delta = 0;

	switch (id) {
	case B1:
		delta = b1 >= b2 ? 1 : b2 / b1;
		delta += self->p;
		self->p = min(delta, self->block_count);
		break;

	case B2:
		delta = b2 >= b1 ? 1 : b1 / b2;
		delta -= self->p;
		self->p = max(delta, 0);
		break;

	default: break;
	}
}

static void add_cache_mru_entry(struct arc_data *self,
				struct cache_meta *entry, __u64 id)
{
	struct cache_meta_list *list = get_list(self, id);

	entry->seq = id;
	cache_meta_list_insert_head(list, entry);
}

static struct cache_meta *remove_cache_lru_entry(struct arc_data *self,
						__u64 id)
{
	struct cache_meta_list *list = get_list(self, id);
	struct cache_meta *entry = cache_meta_list_remove_tail(list);

	if (id == T1 || id == T2)	/* we need to sync if dirty */
		if (entry && entry->dirty)
			cache_sync_block(self->cache, entry->block);

	return entry;
}

static inline void make_ghost_entry(struct arc_data *self,
					__u64 from, __u64 to)
{
	struct cache_meta *entry = remove_cache_lru_entry(self, from);
	add_cache_mru_entry(self, entry, to);
}

static void replace(struct arc_data *self, struct cache_meta *entry)
{
	__u64 sizeT1 = get_list_size(self, T1);

	if (sizeT1 > 0 && (sizeT1 > self->p ||
			(entry && (entry->seq == B2 && sizeT1 == self->p))))
		make_ghost_entry(self, T1, B1);
	else
		make_ghost_entry(self, T2, B2);

	self->cache->stat_replacements++;
}

static struct cache_meta *search_block(struct arc_data *self, __u64 block)
{
	__u64 i;
	struct cache_meta *entry = NULL;

	for (i = 0; i < self->block_count; i++) {
		entry = &self->block_info[i];

		if (entry->block == block) {
			self->cache->stat_hits++;
			return entry;
		}
	}

	self->cache->stat_misses++;

	return NULL;
}

static struct cache_meta *get_free_block(struct arc_data *self)
{
	__u64 i;
	struct cache_meta *entry = NULL;

	for (i = 0; i < self->block_count; i++) {
		entry = &self->block_info[i];

		if (entry->block == BLOCK_INVALID)
			return entry;
	}

	return NULL;	/* this should NOT happen!! */
}

static int arc_init(struct local_cache *cache)
{
	__u64 i;
	__u64 block_count = cache->local->ram->block_count;
	struct arc_data *self = NULL;

	self = malloc(sizeof(struct arc_data)
			+ sizeof(struct cache_meta) * block_count * 2);
	if (self) {
		self->p = 0;
		self->block_count = block_count;

		cache_meta_list_init(&self->t1);
		cache_meta_list_init(&self->b1);
		cache_meta_list_init(&self->t2);
		cache_meta_list_init(&self->b2);

		for (i = 0; i < block_count * 2; i++) {
			struct cache_meta *tmp = &self->block_info[i];

			init_cache_entry(tmp);
			tmp->index = i;
		}

		self->cache = cache;
		cache->private = self;
	}
	else
		return -ENOMEM;

	return 0;
}

static void arc_exit(struct local_cache *cache)
{
	if (cache && cache->private)
		free(cache->private);
}

static int do_arc(struct arc_data *self, __u64 block, int type)
{
	int res = 0;
	__u64 enroll = 0;
	struct local_cache *cache = self->cache;
	struct cache_meta *entry = search_block(self, block);

	if (entry) {
		enroll = entry->seq;

		switch (enroll) {
		case T1:
		case T2:
			(void) cache_meta_list_remove(get_list(self, enroll),
							entry);
			add_cache_mru_entry(self, entry, T2);
			break;

		case B1:
		case B2:
			res++;

			(void) cache_meta_list_remove(get_list(self, enroll),
							entry);

			adaptation(self, enroll);
			replace(self, entry);

			cache_fetch_block(cache, block);
			add_cache_mru_entry(self, entry, T2);
			break;

		default: break;	/* BUG!! */
		}
	}
	else {
		__u64 sizeT1 = get_list_size(self, T1);
		__u64 sizeB1 = get_list_size(self, B1);
		__u64 sizeT2 = get_list_size(self, T2);
		__u64 sizeB2 = get_list_size(self, B2);
		__u64 c = self->block_count;

		res++;

		if (sizeT1 + sizeT2 == c) {
			if (sizeT1 < c) {
				entry = remove_cache_lru_entry(self, B1);
				init_cache_entry(entry);
				replace(self, NULL);
			}
			else {
				entry = remove_cache_lru_entry(self, T1);
				init_cache_entry(entry);
			}
		}
		else { /* sizeT1 + sizeB1 < c */
			__u64 sum = sizeT1 + sizeT2 + sizeB1 + sizeB2;

			if (sum >= c) {
				if (sum == 2 * c) {
					entry = remove_cache_lru_entry(self, B2);
					init_cache_entry(entry);
				}
				replace(self, NULL);
			}
		}

		entry = get_free_block(self);
		cache_fetch_block(cache, block);

		arc_entry_init(entry, block, type);
		add_cache_mru_entry(self, entry, T1);
	}

	return res;
}

static int arc_rw_block(struct local_cache *cache, struct io_request *req)
{
	int res = 0;
	__u64 i;
	struct arc_data *self = (struct arc_data *) cache->private;

	for (i = 0; i < req->len; i++)
		res += do_arc(self, req->offset + i, req->type);

	return res;
}

static void arc_dump(struct local_cache *cache, FILE *fp)
{
}

struct local_cache_ops arc_cache_ops = {
	.init		= &arc_init,
	.exit		= &arc_exit,
	.read_block	= &arc_rw_block,
	.write_block	= &arc_rw_block,
	.dump		= &arc_dump,
};


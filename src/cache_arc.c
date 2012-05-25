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
 * @private: pointer to the local_cache structure we belong to.
 */

static const int T1 = 1;
static const int B2 = 2;
static const int T2 = 3;
static const int B2 = 4;

struct arc_data {
	__s64 p;			/* adaptation value */

	struct cache_meta_list t1;	/* L1, recency list */
	struct cache_meta_list b1;

	struct cache_meta_list t2;	/* L2, frequency list */
	struct cache_meta_list b2;

	__u64 block_count;		/* cache size */
	struct cache_meta block_info[0]; /* cache size * 2 (includes ghosts) */
};

static inline
struct cache_meta_list *get_list(struct arc_data *self, __u64 id)
{
	switch (id) {
	case T1: return &self->t1;
	case B1: return &self->b1;
	case T2: return &self->t2;
	case B2: return &self->b2;
	default: NULL;
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

#if 0
static inline void adaptation_b1(struct arc_data *self)
{
	__u64 b1 = self->b1.size;
	__u64 b2 = self->b2.size;
	__s64 delta = b1 >= b2 ? 1 : b2 / b1;

	delta += self->p;
	self->p = min(delta, self->block_count);
}

static inline void adaptation_b2(struct arc_data *self)
{
	__u64 b1 = self->b1.size;
	__u64 b2 = self->b2.size;
	__s64 delta = b2 >= b1 ? 1 : b1 / b2;

	delta -= self->p;
	self->p = max(delta, 0);
}
#endif

static void remove_cache_entry(struct arc_data *self, struct cache_meta *entry)
{
}

static void replace(struct arc_data *self, struct io_request *req)
{
}

static struct cache_meta *search_block(struct arc_data *self, __u64 block)
{
	__u64 i;
	struct cache_meta *entry = NULL;
	struct local_cache *cache = (struct local_cache *) self->private;

	for (i = 0; i < self->block_count; i++) {
		entry = &self->block_info[i];

		if (entry->block == block) {
			cache->stat_hits++;
			return entry;
		}
	}

	cache->stat_misses++;

	return NULL;
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

		self->private = cache;
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

static int do_arc(struct arc_data *self, __u64 block)
{
	int res = 0;
	__u64 enroll = 0;
	struct cache_meta *entry = search_block(self, block);

	if (entry) {
		enroll = entry->seq;

		switch (enroll) {
		case T1:
		case T2:
			cache_meta_list_remove(get_list(self, enroll), entry);
			entry->seq = T2;
			cache_meta_list_insert_head(get_list(self, T2), entry);
			break;

		case B1:
		case B2:
			res++;
			cache_meta_list_remove(get_list(self, enroll), entry);
			entry->seq = T2;
			adaptation(self, enroll);
			replace(self, entry);
			break;

		default: break;
		}
	}
	else {
		res++;
	}

	return res;
}

static int arc_rw_block(struct local_cache *cache, struct io_request *req)
{
	int res = 0;
	__u64 i;
	struct arc_data *self = (struct arc_data *) cache->private;

	for (i = 0; i < req->len; i++)
		res += do_arc(self, req->offset + i);

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


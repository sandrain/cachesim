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
#include <assert.h>
#include <sys/queue.h>

#include "cachesim.h"
#include "cache_util.h"

#define	_DEBUG_LIRS

/** LIRS implementation.
 * There is no limitation in the LIRS stack size, although it's not realistic.
 */

static const __u16 B_L = 1;
static const __u16 B_H = 2;
static const __u16 B_R = 4;
static const __u16 B_S = 8;

struct lirs_meta {
	struct cache_meta binfo;

	__u16 type;
	__u16 flags; 
	__u64 irr;

	TAILQ_ENTRY(lirs_meta) s;
	TAILQ_ENTRY(lirs_meta) q;
};

struct lirs_data {
	__u64 block_count;
	__u64 total_block_count;
	__u64 nlirs_max;
	__u64 seq;
	__u64 alloc_seq;
	__u64 nlirs;
	__u64 nresidents;

	struct local_cache *cache;

	TAILQ_HEAD(stack_s, lirs_meta) s;
	TAILQ_HEAD(stack_q, lirs_meta) q;

	struct hash_table *htable;
	struct lirs_meta block_info[0];
};

static void stack_prune(struct lirs_data *self)
{
	struct lirs_meta *current;

	while (1) {
		current = TAILQ_FIRST(&self->s);
		if (current->type == B_L)
			break;

		TAILQ_REMOVE(&self->s, current, s);
		current->flags &= ~B_S;
		if ((current->type & B_R) == 0)
			current->type &= ~B_H;
	}
}

static void reclaim(struct lirs_data *self)
{
	struct lirs_meta *entry;

	if (self->nresidents <= self->block_count)
		return;

	entry = TAILQ_FIRST(&self->q);
	TAILQ_REMOVE(&self->q, entry, q);
	entry->type &= ~B_R;
	self->nresidents--;
}

static void reclaim_lir(struct lirs_data *self)
{
	struct lirs_meta *entry;
	struct cache_meta *tmp;

	if (self->nlirs <= self->block_count)
		return;

	entry = TAILQ_FIRST(&self->s);
	tmp = (struct cache_meta *) entry;
	entry->type = B_H | B_R;
	entry->flags &= ~B_S;
	TAILQ_REMOVE(&self->s, entry, s);
	TAILQ_INSERT_TAIL(&self->q, entry, q);
	self->nlirs--;
	stack_prune(self);
}

/** we count the number of blocks offline, so we always have meta blocks to
 * allocate and this function should not fail!!
 */
static inline
struct lirs_meta *alloc_next_block(struct lirs_data *self)
{
	assert(self->alloc_seq <= self->total_block_count);
	return &self->block_info[self->alloc_seq++];
}

static inline
struct lirs_meta *search_block(struct lirs_data *self, __u64 block)
{
	return hash_table_search(self->htable, &block, sizeof(block));
}

static int do_lirs(struct lirs_data *self, __u64 block, int type)
{
	struct local_cache *cache = self->cache;
	struct lirs_meta *entry = NULL;
	struct cache_meta *tmp;

	entry = search_block(self, block);

	if (!entry) {
		entry = alloc_next_block(self);
		entry->binfo.block = block;
		entry->irr = BLOCK_INVALID;
	}
	else
		entry->irr = self->seq - entry->binfo.seq - 1;

	entry->binfo.seq = self->seq;
	tmp = (struct cache_meta *) entry;

	if (entry->type == B_L) {
		TAILQ_REMOVE(&self->s, entry, s);
		TAILQ_INSERT_TAIL(&self->s, entry, s);
		stack_prune(self);

		cache->stat_hits++;
		return 0;
	}

	if (entry->type == (B_H | B_R)) {
		if (entry->flags & B_S) {
			TAILQ_REMOVE(&self->s, entry, s);
			TAILQ_REMOVE(&self->q, entry, q);
			entry->type = B_L;
			self->nlirs++;
			reclaim_lir(self);
		}
		else {
			TAILQ_REMOVE(&self->q, entry, q);
			TAILQ_INSERT_TAIL(&self->q, entry, q);
		}

		TAILQ_INSERT_TAIL(&self->s, entry, s);
		entry->flags |= B_S;

		cache->stat_hits++;
		return 0;
	}

	cache->stat_misses++;
	self->nresidents++;
	reclaim(self);

	if (entry->type == 0) {
		hash_table_insert(self->htable, &block, sizeof(block), entry);

		if (self->nlirs < self->nlirs_max) {
			entry->type = B_L;
			TAILQ_INSERT_TAIL(&self->s, entry, s);
			entry->flags |= B_S;
			self->nlirs++;
		}
		else
			entry->type = B_H;
	}

	if (entry->type == B_H) {
		if (entry->flags & B_S) {
			TAILQ_REMOVE(&self->s, entry, s);
			entry->type = B_L;
			self->nlirs++;
			reclaim_lir(self);
		}
		else {
			entry->type = B_R;
			TAILQ_INSERT_TAIL(&self->q, entry, q);
		}

		TAILQ_INSERT_TAIL(&self->s, entry, s);
		entry->flags |= B_S;
	}

	return 1;
}

int lirs_init(struct local_cache *cache)
{
	struct lirs_data *self;
	struct hash_table *htable;
	struct lirs_meta *current;
	size_t memsize = 0;
	__u64 i, block_count, total_block_count;

	block_count = cache_get_block_count(cache);
	total_block_count = node_get_unique_block_count(NULL);

	htable = hash_table_init(total_block_count);
	if (!htable)
		return -errno;

	memsize += sizeof(*self);
	memsize += sizeof(struct lirs_meta) * total_block_count;

	self = malloc(memsize);
	if (!self) {
		hash_table_exit(htable);
		return -errno;
	}

	self->block_count = block_count;
	self->total_block_count = total_block_count;
	self->seq = 0;
	self->alloc_seq = 0;
	self->nlirs = 0;
	self->nresidents = 0;
	self->htable = htable;
	self->nlirs_max = block_count * 90 / 100;

	TAILQ_INIT(&self->s);
	TAILQ_INIT(&self->q);

	for (i = 0; i < total_block_count; i++) {
		current = &self->block_info[i];
		init_cache_entry((struct cache_meta *) current);
		current->binfo.index = i;
		current->type = 0;
		current->flags = 0;
		current->irr = BLOCK_INVALID;
	}

	self->cache = cache;
	cache->private = self;

	return 0;
}

void lirs_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct lirs_data *self = (struct lirs_data *) cache->private;

		if (self->htable)
			hash_table_exit(self->htable);

		free(self);
	}
}

static inline void dump_lirs_block(struct lirs_meta *entry, FILE *fp)
{
	fprintf(fp, "%llu (%d, %d, irr = %llu)\n",
			entry->binfo.block,
			entry->type, entry->flags, entry->irr);
}

#ifdef	_DEBUG_LIRS
void lirs_dump(struct local_cache *cache, FILE *fp);
#endif

int lirs_rw_block(struct local_cache *cache, struct io_request *req)
{
	int type, res = 0;
	__u64 i, block;
	struct lirs_data *self = (struct lirs_data *) cache->private;

	for (i = 0; i < req->len; i++) {
		block = req->offset + i;
		type = req->type == IOREQ_TYPE_WRITE ? 1 : 0;

		self->seq++;

		res += do_lirs(self, block, type);
#ifdef	_DEBUG_LIRS
		lirs_dump(cache, cachesim_config->output);
		getchar();
#endif
	}

	return res;
}

void lirs_dump(struct local_cache *cache, FILE *fp)
{
	struct lirs_data *self = (struct lirs_data *) cache->private;
	struct lirs_meta *current;

	fprintf(fp, "\nlirs = %llu, residents = %llu",
			self->nlirs, self->nresidents);

	fprintf(fp, "\n** stack S (lru to mru) **\n");

	TAILQ_FOREACH(current, &self->s, s) {
		dump_lirs_block(current, fp);
	}

	fprintf(fp, "\n** stack Q (lru to mru) **\n");

	TAILQ_FOREACH(current, &self->q, q) {
		dump_lirs_block(current, fp);
	}
}

struct local_cache_ops lirs_cache_ops = {
	.init		= &lirs_init,
	.exit		= &lirs_exit,
	.read_block	= &lirs_rw_block,
	.write_block	= &lirs_rw_block,
	.dump		= &lirs_dump,
};


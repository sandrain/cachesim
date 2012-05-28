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

#include "cache.h"
#include "cache_util.h"

struct opt_data {
	__u64 block_count;
	__u64 alloc_seq;
	__u64 seq;

	struct local_cache *cache;

	FILE *fp;

	struct hash_table *htable;
	struct pqueue *pq;
	struct cache_meta *block_info[0];
};

static int opt_compare(const struct cache_meta *e1,
			const struct cache_meta *e2)
{
	__u64 e1_dist = (__u64) e1->private;
	__u64 e2_dist = (__u64) e2->private;

	if (e1_dist > e2_dist)
		return 1;
	else if (e1_dist < e2_dist)
		return -1;
	else
		return 0;
}

static __u64 find_next_reference(struct opt_data *self, __u64 block)
{
	__u64 i, count = 0;
	__u64 offset, len, sequence;
	int type;
	FILE *fp = self->fp;
	char lbuf[64];

	rewind(fp);

	while (fgets(lbuf, 63, fp) != NULL) {
		if (lbuf[0] == '#' || isspace(lbuf[0]))
			continue;

		sscanf(lbuf, "%llu %llu %c %llu",
				&offset, &len, &type, &sequence);

		count += len;

		if (count < self->seq)
			continue;

		/**** TODO ****/
	}
}

static struct cache_meta *get_free_block(struct opt_data *self)
{
	__u64 pos;

	if (self->alloc_seq < self->block_count) {
		pos = self->alloc_seq++;
		return &self->block_info[pos];
	}

	return NULL;
}

static inline struct cache_meta *select_victim(struct opt_data *self)
{
	return pqueue_dequeue(self->pq);
}

static void replacement(struct opt_data *self, __u64 block, int dirty)
{
	struct cache_meta *entry = NULL;

	entry = get_free_block(self);
	if (!entry) {
		entry = select_victim(self);
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
	entry->private = (void *) find_next_reference(self, block);

	hash_table_insert(self->htable, &block, sizeof(block), entry);
	pqueue_enqueue(self->pq, entry);
}

static struct cache_meta *search_block(struct opt_data *self)
{
	struct cache_meta *entry = NULL;

	entry = hash_table_search(self->htable, &block, sizeof(block));
	if (entry) {
		cache->stat_hits++;

		/** definitely, the priority of this block should be greater,
		 * which means that the chance of being chosen as a victim gets
		 * higher. */
		entry->private = (void *) find_next_reference(self,
							entry->block);
		pqueue_fix_up(self->pq, entry);
	}
	else
		cache->stat_misses++;

	return entry;
}

int opt_init(struct local_cache *cache)
{
	int res = 0;
	__u64 i, block_count;
	FILE *fp;
	struct opt_data *self = NULL;
	struct hash_table *htable;
	struct pqueue *pq;

	block_count = cache_get_block_count(cache);

	fp = freopen(cache->local->app->trace);
	if (!fp) {
		res = -errno;
		goto out;
	}

	htable = hash_table_init(block_count);
	if (!htable) {
		res = -ENOMEM;
		goto out_fp;
	}

	pq = pqueue_init(block_count, &opt_compare);
	if (!pq) {
		res = -ENOMEM;
		goto out_hash;
	}

	self = malloc(sizeof(*self) +
			sizeof(struct cache_meta *) * block_count);
	if (!self) {
		res = -ENOMEM;
		goto out_queue;
	}

	self->block_count = block_count;
	self->alloc_seq = 0;
	self->seq = 0;
	self->cache = cache;
	self->fp = fp;
	self->htable = htable;
	self->pq = pq;

	for (i = 0; i < block_count; i++) {
		struct cache_meta *current = &self->block_info[i];

		init_cache_entry(current);
		current->index = i;
	}

	cache->private = self;

	return 0;

out_queue:
	pqueue_exit(pq);
out_hash:
	hash_table_exit(htable);
out_fp:
	fclose(fp);
out:
	return res;
}

void opt_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct opt_data *self = (struct opt_data *) cache->private;

		if (self->pq)
			pqueue_exit(self->pq);
		if (self->htable)
			hash_table_exit(self->htable);
		if (self->fp)
			fclose(self->fp);

		free(self);
	}
}

int opt_rw_block(struct local_cache *cache, struct io_request *req)
{
	__u64 i, block;
	int res = 0, dirty;
	struct cache_meta *entry = NULL;
	struct opt_data *self = (struct opt_data *) cache->private;

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

	return res;
}

void opt_dump(struct local_cache *cache, FILE *fp)
{
}

struct local_cache_ops opt_ops = {
	.init		= &opt_init,
	.exit		= &opt_exit,
	.read_block	= &opt_rw_block,
	.write_block	= &opt_rw_block,
	.dump		= &opt_dump,
};


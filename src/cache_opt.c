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
#include <ctype.h>

#include "cache.h"
#include "cache_util.h"

#if 0
#define	_DEBUG_OPT
#endif

/** This is applied and fixed!!
 *This implementation takes too much time to finish; every time we access
 * block, we scan the file and search the next reference of a block. Instead of
 * this way, we can initially build a table for each block's reference then
 * continue the actuall processing. That method only need to scan the file once
 * and can decrease the running time significantly. Memory space overhead is
 * the tradeoff.
 */

struct block_reference {
	__u64 block;
	__u64 seq;
	struct block_reference *last;
	struct block_reference *next;
};

struct block_table {
	__u64 block_count;
	__u64 request_count;
	struct hash_table *htable;
	struct block_reference ref_table[0];
};

struct opt_data {
	__u64 block_count;
	__u64 request_count;
	__u64 alloc_seq;
	__u64 seq;

	struct local_cache *cache;

	struct block_table *btable;
	struct hash_table *htable;
	struct pqueue *pq;
	struct cache_meta block_info[0];
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

static __u64 get_unique_block_count(void)
{
	FILE *fp;
	char buf[128];
	__u64 count;

	sprintf(buf, "cat %s|grep -v '^#'|cut -f1 -d' '|sort -u|wc -l",
			cachesim_config->trace_file);
	fp = popen(buf, "r");
	fgets(buf, 127, fp);
	count = atoll(buf);
	fclose(fp);

	return count;
}

static __u64 get_request_count(void)
{
	FILE *fp;
	char buf[128];
	__u64 count;

	sprintf(buf, "cat %s|grep -v '^#'|cut -f1 -d' '|wc -l",
			cachesim_config->trace_file);
	fp = popen(buf, "r");
	fgets(buf, 127, fp);
	count = atoll(buf);
	fclose(fp);

	return count;
}

static struct block_table *build_block_table(void)
{
	struct hash_table *htable = NULL;
	struct block_table *bt = NULL;
	__u64 bcount = get_unique_block_count();
	__u64 rcount = get_request_count();
	__u64 pos = 0;
	char line[64];
	FILE *fp;
	struct block_reference *current, *tmp;

	fp = fopen(cachesim_config->trace_file, "r");
	if (!fp)
		return NULL;

	htable = hash_table_init(bcount);
	if (!htable)
		return NULL;

	bt = malloc(sizeof(*bt) + sizeof(struct block_reference) * rcount);
	if (!bt) {
		hash_table_exit(htable);
		return NULL;
	}

	bt->block_count = bcount;
	bt->request_count = rcount;
	bt->htable = htable;

	rewind(fp);

	rcount = 0;

	while (fgets(line, 63, fp) != NULL) {
		char type;
		__u64 i, offset, len, seq;

		if (line[0] == '#' || isspace(line[0]))
			continue;

		sscanf(line, "%llu %llu %c %llu",
				&offset, &len, &type, &seq);

		for (i = 0; i < len; i++) {
			__u64 block = offset + i;
			current = &bt->ref_table[pos++];
			current->block = block;
			current->seq = ++rcount;
			current->last = NULL;
			current->next = NULL;

			tmp = hash_table_search(htable, &block, sizeof(block));
			if (!tmp) {
				hash_table_insert(htable, &block,
						sizeof(block), current);
				continue;
			}

			while (tmp->next)
				tmp = tmp->next;

			tmp->next = current;
		}
	}

	fclose(fp);

	return bt;
}

static void destory_block_table(struct block_table *bt)
{
	if (bt) {
		if (bt->htable)
			hash_table_exit(bt->htable);
		free(bt);
	}
}

static __u64 find_next_reference(struct opt_data *self, __u64 block)
{
	struct hash_table *ht = self->btable->htable;
	struct block_reference *ref;

	ref = hash_table_search(ht, &block, sizeof(block));
	/** BUG if the ref not found!! */

	if (!ref->last)
		ref->last = ref;

	ref->last = ref->last->next;

	return ref->last ? ref->last->seq : BLOCK_INVALID;
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

static struct cache_meta *search_block(struct opt_data *self, __u64 block)
{
	struct cache_meta *entry = NULL;
	struct local_cache *cache = self->cache;

	entry = hash_table_search(self->htable, &block, sizeof(block));
	if (entry) {
		cache->stat_hits++;

		/** definitely, the priority of this block should be greater,
		 * which means that the chance of being chosen as a victim gets
		 * higher. */
		entry->private = (void *) find_next_reference(self,
							entry->block);
		entry->seq = self->seq;
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
	struct opt_data *self = NULL;
	struct hash_table *htable;
	struct block_table *btable;
	struct pqueue *pq;

	block_count = cache_get_block_count(cache);

	btable = build_block_table();
	if (btable == NULL) {
		res = -errno;
		goto out;
	}

	htable = hash_table_init(block_count);
	if (!htable) {
		res = -ENOMEM;
		goto out;
	}

	pq = pqueue_init(block_count, &opt_compare);
	if (!pq) {
		res = -ENOMEM;
		goto out_hash;
	}

	self = malloc(sizeof(*self) +
			sizeof(struct cache_meta) * block_count);
	if (!self) {
		res = -ENOMEM;
		goto out_queue;
	}

	self->block_count = block_count;
	self->alloc_seq = 0;
	self->seq = 0;
	self->cache = cache;
	self->btable = btable;
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
out:
	return res;
}

void opt_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct opt_data *self = (struct opt_data *) cache->private;

		if (self->btable)
			destory_block_table(self->btable);

		if (self->pq)
			pqueue_exit(self->pq);

		if (self->htable)
			hash_table_exit(self->htable);

		free(self);
	}
}

#ifdef	_DEBUG_OPT
void opt_dump(struct local_cache *cache, FILE *fp);
#endif

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

#ifdef	_DEBUG_OPT
	opt_dump(cache, cachesim_config->output);
	(void) getchar();
#endif

	return res;
}

void opt_dump(struct local_cache *cache, FILE *fp)
{
	__u64 i;
	struct opt_data *self = (struct opt_data *) cache->private;
	struct cache_meta *current;

	for (i = 0; i <  self->block_count; i++) {
		current = &self->block_info[i];

		if (current->block == BLOCK_INVALID)
			continue;

		fprintf(fp,
			"[%5llu] %d, %llu, seq = %llu, next = %llu\n",
			i, current->dirty, current->block,
			current->seq, (__u64) current->private);
	}
}

struct local_cache_ops opt_cache_ops = {
	.init		= &opt_init,
	.exit		= &opt_exit,
	.read_block	= &opt_rw_block,
	.write_block	= &opt_rw_block,
	.dump		= &opt_dump,
};


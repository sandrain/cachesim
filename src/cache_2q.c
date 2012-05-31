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

#include "cachesim.h"
#include "cache_util.h"

#define	_DEBUG_2Q

/** Kin, Kout params; according to the original paper, the reasonable
 * parameters are Kin = 0.3c and Kout = 0.5c where c denotes the cache
 * capacity.
 */
#define	KIN		30
#define	KOUT		50

/** We have to keep track of which block belongs to which list. To do that, we
 * use (void *) @private field in struct cache_meta. Initially, it is set to 0.
 */
const static __u64 A1IN  = 1;
const static __u64 A1OUT = 2;
const static __u64 AM    = 4;

struct twoq_data {
	__u64 block_count;
	__u64 kin;
	__u64 kout;
	__u64 ghost_count;
	__u64 seq;
	__u64 alloc_seq;

	struct cache_meta_list a1in;
	struct cache_meta_list a1out;
	struct cache_meta_list am;

	struct local_cache *cache;
	struct hash_table *htable;
	struct cache_meta block_info[0];
};

static struct cache_meta *get_free_block(struct twoq_data *self)
{
	if (self->alloc_seq < self->block_count + self->ghost_count)
		return &self->block_info[self->alloc_seq++];
	assert(0);
}

static struct cache_meta *search_block(struct twoq_data *self, __u64 block)
{
	return hash_table_search(self->htable, &block, sizeof(block));
}

#if 0
static inline __u64 get_total_used_blocks(struct twoq_data *self)
{
	return self->a1in.size + self->am.size + self->a1out.size;
}
#endif

static inline __u64 get_resident_blocks(struct twoq_data *self)
{
	return self->a1in.size + self->am.size;
}

static struct cache_meta *reclaim(struct twoq_data *self)
{
	struct cache_meta *entry = NULL;

	if (get_resident_blocks(self) < self->block_count)
		return get_free_block(self);

	if (self->a1in.size > self->kin) {	/* kick out from A1IN */
		entry = cache_meta_list_remove_tail(&self->a1in);
		if (entry->dirty)
			cache_sync_block(self->cache, entry->block);

		entry->private = (void *) A1OUT;
		cache_meta_list_insert_head(&self->a1out, entry);

		if (self->a1out.size > self->kout) {
			entry = cache_meta_list_remove_tail(&self->a1out);
			init_cache_entry(entry);
		}

		return get_free_block(self);
	}

	/* kick out from AM */
	entry = cache_meta_list_remove_tail(&self->am);
	cache_sync_block(self->cache, entry->block);
	hash_table_delete(self->htable, &entry->block, sizeof(entry->block));
	init_cache_entry(entry);

	return get_free_block(self);
}

static inline void init_twoq_entry(struct twoq_data *self,
			struct cache_meta *entry, __u64 block, int type)
{
	entry->dirty = type;
	entry->block = block;
	entry->seq = self->seq;
	entry->private = (void *) A1IN;	/* New entries always go to a1in */
}

static int do_twoq(struct twoq_data *self, __u64 block, int type)
{
	struct local_cache *cache = self->cache;
	struct cache_meta *entry = NULL;
	__u64 list = 0;

	entry = search_block(self, block);

	if (!entry) {
		cache->stat_misses++;
		cache_fetch_block(cache, block);

		entry = reclaim(self);
		init_twoq_entry(self, entry, block, type);
		cache_meta_list_insert_head(&self->a1in, entry);
		hash_table_insert(self->htable, &block, sizeof(block), entry);

		return 1;
	}

	list = (__u64) entry->private;

	if (list == A1OUT) {	/* ghost hit */
		cache->stat_misses++;
		cache_fetch_block(cache, block);

		cache_meta_list_remove(&self->a1out, entry);
		entry->dirty = type;
		entry->private = (void *) AM;
		cache_meta_list_insert_head(&self->am, entry);
		(void) reclaim(self);

		return 1;
	}

	if (list == AM) {
		entry = cache_meta_list_remove(&self->am, entry);
		cache_meta_list_insert_head(&self->am, entry);
	}

	/* do nothing about the rest; A1 hit */
	cache->stat_hits++;

	return 0;
}

static void twoq_dump(struct local_cache *cache, FILE *fp)
{
	__u64 i;
	struct twoq_data *self = cache->private;
	struct cache_meta *current;

	fprintf(fp, "\n2Q: kin=%llu, kout=%llu", self->kin, self->kout);
	fprintf(fp, "\n===========================[%llu]", self->seq);

	fprintf(fp, "\n** AM (%llu entries) **\n", self->am.size);
	for (i = 0, current = self->am.head; current;
			current = current->next, i++)
		generic_cache_entry_dump(i, current, fp);

	fprintf(fp, "\n** A1IN (%llu entries) **\n", self->a1in.size);
	for (i = 0, current = self->a1in.head; current;
			current = current->next, i++)
		generic_cache_entry_dump(i, current, fp);

	fprintf(fp, "\n** A1OUT (%llu entries) **\n", self->a1out.size);
	for (i = 0, current = self->a1out.head; current;
			current = current->next, i++)
		generic_cache_entry_dump(i, current, fp);
}

static int twoq_rw_block(struct local_cache *cache, struct io_request *req)
{
	__u64 i;
	int type, res = 0;
	struct twoq_data *self = (struct twoq_data *) cache->private;

	for (i = 0; i < req->len; i++) {
		type = req->type == IOREQ_TYPE_WRITE ? 1 : 0;

		self->seq++;
		res += do_twoq(self, req->offset + i, type);

#ifdef	_DEBUG_2Q
		twoq_dump(cache, cachesim_config->output);
		getchar();
#endif
	}

	return res;
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
	self->kin = block_count * KIN / 100;
	self->kout = block_count - self->kin;
	self->ghost_count = block_count * KOUT / 100;
	self->seq = 0;
	self->alloc_seq = 0;
	self->htable = htable;

	for (i = 0; i < total_block_count; i++) {
		struct cache_meta *current = &self->block_info[i];

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
		struct twoq_data *self = (struct twoq_data *) cache->private;

		if (self->htable)
			hash_table_exit(self->htable);
		free(self);
	}
}

struct local_cache_ops twoq_cache_ops = {
	.init		= &twoq_init,
	.exit		= &twoq_exit,
	.read_block	= &twoq_rw_block,
	.write_block	= &twoq_rw_block,
	.dump		= &twoq_dump,
};


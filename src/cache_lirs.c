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

/** Block types */
static const int LIR	= 1;	/* LIR block */
static const int HIR	= 2;	/* resident HIR block */
static const int HIRN	= 3;	/* non-resident HIR block */

struct lirs_meta {
	struct cache_meta binfo;

	int type;
	__u64 irr;
};

struct lirs_data {
	__u64 block_count;
	__u64 total_block_count;
	__u64 seq;

	struct local_cache *cache;

	struct cache_meta_list s;
	struct cache_meta_list q;

	struct hash_table *htable;
	struct lirs_meta block_info[0];
};

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
	self->htable = htable;

	cache_meta_list_init(&self->s);
	cache_meta_list_init(&self->q);

	for (i = 0; i < total_block_count; i++) {
		current = &self->block_info[i];
		init_cache_entry((struct cache_meta *) current);
		current->type = 0;
		current->irr = BLOCK_INVALID;
	}

	self->cache = cache;
	cache->private = self;

	return 0;
}

void lirs_exit(struct local_cache *cache)
{
	if (cache && cache->private) {
		struct lirs_data *self = (struct lirs_data *) cache;

		if (self->htable)
			hash_table_exit(self->htable);

		free(self);
	}
}

int lirs_rw_block(struct local_cache *cache, struct io_request *req)
{
}

void lirs_dump(struct local_cache *cache, FILE *fp)
{
}

struct local_cache_ops lirs_cache_ops = {
	.init		= &lirs_init,
	.exit		= &lirs_exit,
	.read_block	= &lirs_rw_block,
	.write_block	= &lirs_rw_block,
	.dump		= &lirs_dump,
};


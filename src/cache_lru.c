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

struct lru_data {
	__u64 block_count;

	struct cache_meta *lru;
	struct cache_meta *mru;

	struct cache_meta block_info[0];
};

static inline void move_to_mru(struct lru_data *self, __u64 index)
{
}

static inline void insert_mru(struct lru_data *self, __u64 index)
{
}

static inline __u64 get_lru(struct lru_data *self)
{
}

static inline __u64 get_mru(struct lru_data *self)
{
}

static int lru_init(struct local_cache *cache)
{
	__u64 i;
	__u64 block_count = cache->local->ram->block_count;
	struct lru_data *self = NULL;

	self = malloc(sizeof(lru_data)
			+ sizeof(struct cache_meta) * block_count);
	if (!self)
		return -ENOMEM;

	self->block_count = block_count;
	self->lru = NULL;
	self->mru = NULL;

	for (i = 0; i < block_count; i++)
		init_cache_entry(&self->block_info[i]);

	cache->private = self;
	return 0;
}

static void lru_exit(struct local_cache *cache)
{
	if (cache && cache->private)
		free(cache->private);
}

static int lru_rw_block(struct local_cache *cache, struct io_request *req)
{
}

static void lru_dump(struct local_cache *cache, FILE *fp)
{
}

static int mru_rw_block(struct local_cache *cache, struct io_request *req)
{
}

struct local_cache_ops lru_cache_ops = {
	.init		= &lru_init,
	.exit		= &lru_exit,
	.read_block	= &lru_rw_block,
	.write_block	= &lru_rw_block,
	.dump		= &lru_dump,
};

struct local_cache_ops mru_cache_ops = {
	.init		= &lru_init,
	.exit		= &lru_exit,
	.read_block	= &mru_rw_block,
	.write_block	= &mru_rw_block,
	.dump		= &lru_dump,
};


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

struct arc_data {
	__s64 p;

	struct cache_meta_list t1;
	struct cache_meta_list b1;

	struct cache_meta_list t2;
	struct cache_meta_list b2;

	__u64 block_count;
	struct cache_meta block_info[0];
};

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

		for (i = 0; i < block_count; i++)
			init_cache_entry(&self->block_info[i]);

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

static int arc_rw_block(struct local_cache *cache, struct io_request *req)
{
	return 0;
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


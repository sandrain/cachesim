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

struct local_cache_ops lru_cache_ops = {
	.init		= &lru_init,
	.exit		= &lru_exit,
	.read_block	= &lru_rw_block,
	.write_block	= &lru_rw_block,
	.dump		= &lru_dump,
};

static int lru_init(struct local_cache *cache)
{
}

static void lru_exit(struct local_cache *cache)
{
}

static struct lru_rw_block(struct local_cache *cache, struct io_request *req)
{
}

static void lru_dump(struct local_cache *cache, FILE *fp)
{
}

struct local_cache_ops mru_cache_ops = {
	.init		= &lru_init,
	.exit		= &lru_exit,
	.read_block	= &mru_rw_block,
	.write_block	= &mru_rw_block,
	.dump		= &lru_dump,
};


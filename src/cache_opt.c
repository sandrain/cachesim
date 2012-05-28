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
	__u64 seq;

	struct hash_table *htable;
	struct cache_meta *block_info[0];
};

int opt_init(struct local_cache *cache)
{
}

void opt_exit(struct local_cache *cache)
{
}

int opt_rw_block(struct local_cache *cache, struct io_request *req)
{
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


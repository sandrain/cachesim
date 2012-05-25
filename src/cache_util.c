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

int cache_sync_block(struct local_cache *cache, __u64 block)
{
	struct io_request req;

	req.type = IOREQ_TYPE_WRITE;
	req.offset = block;
	req.len = 1;

	return local_cache_sync_block(cache, &req);
}

int cache_fetch_block(struct local_cache *cache, __u64 block)
{
	struct io_request req;

	req.type = IOREQ_TYPE_READ;
	req.offset = block;
	req.len = 1;

	return local_cache_fetch_block(cache, &req);
}

int cache_rw_cache_dev(struct local_cache *cache, __u64 block, int type)
{
	struct io_request req;

	req.type = type;
	req.offset = block;
	req.len = 1;

	return storage_rw_block(cache->local->ram, &req);
}


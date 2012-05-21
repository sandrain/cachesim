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
#include <stdlib.h>
#include <string.h>

#include "cachesim.h"

struct node *node_init(__u32 id, struct ioapp *app, struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd,
		struct node_operations *ops, void *private)
{
	struct node *self;

	if (!ram || !ops) {
		errno = EINVAL;
		return NULL;
	}

	if (!id && !app) {
		errno = EINVAL;
		return NULL;
	}

	self = malloc(sizeof(*self));
	if (self) {
		self->id = id;
		self->app = app;
		self->cache = cache;
		self->ram = ram;
		self->ssd = ssd;
		self->hdd = hdd;
		self->ops = ops;
		self->private = private;
	}

	return self;
}

void node_exit(struct node *self)
{
	if (self)
		free(self);
}

int node_service_ioapp(struct node *self)
{
	int res = 0;
	struct io_request req;

	res = ioapp_next_request(self->app, &req);
	if (res == IOREQ_TYPE_EOF)
		return res;

	res = local_cache_rw_block(self->cache, &req);
	if (res == CACHE_HIT)
		return res;

	/** now we should request to the pfs, which requires synchronization.
	 */
	pfs_lock();
	res = node_pfs_rw_block(self->pfs, &req);
	pfs_unlock();

	local_cache_insert_block(self->cache, &req);

	return res;
}

int node_pfs_rw_block(struct node *self, struct io_request *req)
{
	int res = 0;

	if (!self || !req)
		return -EINVAL;

	res = local_cache_rw_block(self->cache, req);
	if (res == CACHE_HIT)
		return res;

	return storage_rw_block(self->hdd, req);
}

void node_get_statistics(struct node *self, struct node_statistics *stat)
{
	if (!self || !stat)
		return;

	memset(stat, 0, sizeof(*stat));

	if (self->ram) {
		stat->ram_reads = self->ram->stat_reads;
		stat->ram_writes = self->ram->stat_writes;
	}

	if (self->ssd) {
		stat->ssd_reads = self->ssd->stat_reads;
		stat->ssd_writes = self->ssd->stat_writes;
	}

	if (self->hdd) {
		stat->hdd_reads = self->hdd->stat_reads;
		stat->hdd_writes = self->hdd->stat_writes;
	}

	if (self->cache) {
		stat->cache_hits = self->cache->stat_hits;
		stat->cache_misses = self->cache->stat_misses;
		stat->cache_replacements = self->cache->stat_replacements;
	}
}


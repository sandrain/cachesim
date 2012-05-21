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
#include <errno.h>

#include "cachesim.h"

extern int errno;

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

static int generic_find_block(struct node *self, __u32 remote, __u64 block)
{
	if (self->id == 0) {
		return -EINVAL;
	}

	if (!self->cache)
		return -ENODEV;

	return self->cache->ops->get_block(self->cache,block);
}

static int generic_read_block(struct node *self, __u32 remote, __u64 block)
{
	if (self->id == 0)
		return self->hdd->ops->read_block(self->hdd, block, 1);
	else
		return self->cache->ops->read_block(self->cache, block);
}

static int generic_write_block(struct node *self, __u32 remote, __u64 block)
{
	if (self->id == 0)
		return self->hdd->ops->write_block(self->hdd, block, 1);
	else
		return self->cache->ops->write_block(self->cache, block);

	return 0;
}

struct node_operations compute_node_operations = {
	.find_block	= &generic_find_block,
	.read_block	= &generic_read_block,
	.write_block	= &generic_write_block,
};

struct node_operations pfs_node_operations = {
	.find_block	= &generic_find_block,
	.read_block	= &generic_read_block,
	.write_block	= &generic_write_block,
};

int node_service_ioapp(struct node *self)
{
	int res = 0;
	struct io_request req;
	struct node *pfs;

	res = ioapp_next_request(self->ioapp, &req);
	if (res == IOREQ_TYPE_EOF)
		return res;

	res = local_cache_rw_block(self->cache, &req);
	if (res == CACHE_HIT)
		return res;

	/** now we should request to the pfs. */
	return node_pfs_rw_block(self->pfs, &req);
}

int node_pfs_rw_block(struct node *self, struct io_request *req)
{
	int res = 0;

	if (!self || !req)
		return -EINVAL;

	res = local_cache_rw_block(self->cache, &req);
	if (res == CACHE_HIT)
		return res;

	return storage_rw_block(self->hdd, &req);
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


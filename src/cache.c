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
#include <string.h>

#include "cachesim.h"

static struct local_cache_ops *cache_ops[] = {
	&none_cache_ops,
	&random_cache_ops,
	&opt_cache_ops,
	&fifo_cache_ops,
	&lru_cache_ops,
	&mru_cache_ops,
	&lfu_cache_ops,
	&arc_cache_ops,
	&lirs_cache_ops,
	&twoq_cache_ops
};

static inline struct local_cache_ops *get_cache_ops(int policy)
{
	return cache_ops[policy];
}

struct local_cache *local_cache_init(struct local_cache *self, int policy,
				struct node *local, struct storage *dev,
				void *source, int stype)
{
	if (!self) {
		errno = EINVAL;
		return NULL;
	}

	memset(self, 0, sizeof(*self));

	self->node = local->id;
	self->policy = policy;
	self->local = local;
	self->cache_dev = dev;
	self->source.type = stype;

	switch (stype) {
	case CACHE_SRC_LOCAL_DEV:
		self->source.s.local_dev = source;
		break;
	case CACHE_SRC_LOCAL_CACHE:
		self->source.s.local_cache = source;
		break;
	case CACHE_SRC_REMOTE:
		self->source.s.remote = source;
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	self->ops = get_cache_ops(policy);
	if (self->ops->init)
		if (self->ops->init(self) != 0)
			return NULL;

	return self;
}

void local_cache_exit(struct local_cache *self)
{
	if (self && self->ops && self->ops->exit)
		self->ops->exit(self);
}

int local_cache_rw_block(struct local_cache *self, struct io_request *req)
{
	int res = 0;

	switch (req->type) {
	case IOREQ_TYPE_ANY:
	case IOREQ_TYPE_READ:
		res = self->ops->read_block(self, req);
		break;
	case IOREQ_TYPE_WRITE:
		res = self->ops->write_block(self, req);
		break;
	default:
		res = -EINVAL;
		break;
	}

	return res;
}

int local_cache_sync_block(struct local_cache *self, struct io_request *req)
{
	int type = self->source.type;

	switch (type) {
	case CACHE_SRC_LOCAL_DEV:
		return storage_rw_block((struct storage *)
					self->source.s.local_dev, req);
	case CACHE_SRC_LOCAL_CACHE:
		return local_cache_rw_block((struct local_cache *)
					self->source.s.local_cache, req);
	case CACHE_SRC_REMOTE:
		return node_rw_block((struct node *)
					self->source.s.remote, req);
	default: return -EINVAL;
	}
}

void local_cache_dump(struct local_cache *self, FILE *fp)
{
	if (self && self->ops && self->ops->dump)
		self->ops->dump(self, fp);
}

/**
 * An example implementation of the cache algorithm. This algorithm redirects
 * all the requests to the stable storage and can be useful to simulate the
 * situation without cache.
 */
static int none_init(struct local_cache *self)
{
	return 0;
}

static void none_exit(struct local_cache *self)
{
	return;
}

static int none_rw_block(struct local_cache *self, struct io_request *req)
{
	return local_cache_sync_block(self, req);
}

static void none_dump(struct local_cache *self, FILE *fp)
{
	return;
}

struct local_cache_ops none_cache_ops = {
	.init		= &none_init,
	.exit		= &none_exit,
	.read_block	= &none_rw_block,
	.write_block	= &none_rw_block,
	.dump		= &none_dump,
};


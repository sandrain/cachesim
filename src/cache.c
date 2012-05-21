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

struct local_cache *local_cache_init(struct local_cache *self,
				__u32 node, int policy, int ndevs,
				struct storage *devs[N_CACHE_DEVS],
				struct node *pfs,
				struct local_cache_ops *ops, void *data)
{
	int i;

	if (!self) {
		errno = EINVAL;
		return NULL;
	}

	if (policy < 0 || policy >= N_CACHE_POLICIES) {
		errno = EINVAL;
		return NULL;
	}

	memset(self, 0, sizeof(*self));

	self->node = node;
	self->policy = policy;
	self->private = data;
	self->ops = ops;
	self->pfs = pfs;

	for (i = 0; i < ndevs; i++)
		self->devs[i] = devs[i];

	return self;
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


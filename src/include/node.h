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
#ifndef	__NODE_H__
#define	__NODE_H__

#include "cachesim.h"

#define	PFS_NODE_ID	0

/**
 * Not used currently..
 */
struct node_operations {
};

/**
 * Abstraction of a node.
 */
struct node {
	__u32 id;			/* id of this node */

	struct ioapp *app;		/* application which runs here */
	struct local_cache *cache;	/* local cache */

	struct storage *ram;		/* storage devices in this node */
	struct storage *ssd;
	struct storage *hdd;

	struct node *pfs;		/* connection to the pfs node */

	void *private;			/* free hook */

	struct node_operations *ops;	/* operations, currently not used */
};

/** these are not used, yet */
extern struct node_operations compute_node_operations;
extern struct node_operations pfs_node_operations;

/**
 * node_init initializes the node instance. NOTE that @cache and @pfs should be
 * set using other functions. It is prefered to use node_init_compute() or
 * node_init_pfs() below instead of directly using this function.
 *
 * @self: memory space which should be allocated by the caller.
 * @id: node id assigned to this node.
 * @app: application instance.
 * @ram: ram device.
 * @ssd: ssd device.
 * @hdd: hdd device.
 * @ops: operation table.
 * @private: free pointer hook, which can be used by the caller.
 *
 * returns initialized node instance. NULL on error.
 *
 * FIXME: this function requires too many arguments.
 */
struct node *node_init(struct node *self,
		__u32 id, struct ioapp *app,
		struct storage *ram, struct storage *ssd, struct storage *hdd,
		struct node_operations *ops, void *private);

/**
 * node_exit de-initializes the node instance.
 *
 * @self: node instance.
 */
static inline void node_exit(struct node *self) {}

/**
 * node_set_local_cache sets the local_cache instance of the given node.
 *
 * @self: node instance.
 * @cache: local_cache instance.
 */
static inline
void node_set_local_cache(struct node *self, struct local_cache *cache)
{
	self->cache = cache;
}

/**
 * node_init_compute is a wrapper of node_init for initializing a computing
 * node. @app, @ram, @ssd, and @hdd should be properly initialized before
 * calling this function.
 *
 * @self: should be allocated by the caller.
 * @id: node id to be set.
 * @app: application instance.
 * @ram: ram device.
 * @ssd: ssd device.
 * @hdd: hdd device.
 *
 * returns node instance on success, NULL otherwise.
 */
static inline
struct node *node_init_compute(struct node *self, __u32 id, struct ioapp *app,
		struct storage *ram, struct storage *ssd, struct storage *hdd)
{
	return node_init(self, id, app, ram, ssd, hdd,
			&compute_node_operations, NULL);
}

/**
 * node_init_pfs is a wrapper for a pfs node. All device arguments should be
 * initialized before calling this. There is no application instance in pfs
 * node. In simulation, only one pfs node should be used and the id of pfs node
 * is always 0.
 *
 * @self: should be allocated by the caller.
 * @ram: ram device.
 * @ssd: ssd device.
 * @hdd: hdd device.
 *
 * return node instance on success, NULL otherwise.
 */
static inline
struct node *node_init_pfs(struct node *self,
		struct storage *ram, struct storage *ssd, struct storage *hdd)
{
	return node_init(self, 0, NULL, ram, ssd, hdd,
			&pfs_node_operations, NULL);
}

/**
 * node_set_pfs sets the pfs instance for a node.
 *
 * @self: node instance.
 * @pfs: pfs instance.
 */
static inline
void node_set_pfs(struct node *self, struct node *pfs)
{
	if (self)
		self->pfs = pfs;
}

/**
 * node_service_ioapp fetches and services block io requests from the
 * @self->app.
 *
 * @self: node instance
 *
 * returns 0.
 */
int node_service_ioapp(struct node *self);

/**
 * node_pfs_rw_block would be used when you need to request block io operation
 * to the pfs node.
 *
 * @pfs: pfs node instance.
 * @req: block io request.
 *
 * returns number of hits in pfs cache, <0 on error.
 */
int node_pfs_rw_block(struct node *pfs, struct io_request *req);

/**
 * node_get_statistics fills @stat structure with current node statistics.
 *
 * @self: node instance.
 * @stat: [out] stat structure to be filled.
 */
void node_get_statistics(struct node *self, struct node_statistics *stat);

#endif	/** __NODE_H__ */


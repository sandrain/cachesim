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
#include <errno.h>

#include "node.h"

struct node *node_init(__u32 id, struct ioapp *app, struct local_cache *cache,
		struct storage *ram, struct storage *ssd, struct storage *hdd,
		struct node_operations *ops)
{
}

struct node *node_init_compute(__u32 id, struct ioapp *app,
		struct storage *ram, struct storage *ssd, struct storage *hdd)
{
}

struct node *node_init_pfs(struct storage *ram, struct storage *ssd,
			struct storage *hdd)
{
}

void node_exit(struct node *self)
{
}


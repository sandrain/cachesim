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
#include "cachesim.h"

static int random_init(struct local_cache *self)
{
	return 0;
}

static int random_exit(struct local_cache *self)
{
	return 0;
}

static int random_read_block(struct local_cache *self, struct io_request *req)
{
	return 0;
}

static int random_write_block(struct local_cache *self, struct io_request *req)
{
	return 0;
}

struct local_cache_ops random_cache_ops = {
	.init		= &random_init,
	.exit		= &random_exit,
	.read_block	= &random_read_block,
	.write_block	= &random_write_block,
};


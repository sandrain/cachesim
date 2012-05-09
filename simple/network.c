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
#include <linux/types.h>

#include "network.h"

struct network *network_init(__u32 n)
{
	struct network *self = malloc(sizeof(*self) + sizeof(__u32) * n * n);

	if (self) {
		memset(self, 0, sizeof(*self) + sizeof(__u32) * n * n);
		self->n = n;
	}

	return self;
}

void network_exit(struct network *self)
{
	if (self) {
		if (self->grid)
			free(self->grid);
		free(self);
	}
}


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
#ifndef	__NETWORK_H__
#define	__NETWORK_H__

/* Simply, the matrix stores the cost between two nodes. */
struct network {
	__u32	n;
	double grid[0];
};

struct network *network_init(__u32 n);

void network_exit(struct network *self);

static inline void set_network_cost(struct network *self,
					__u32 n1, __u32 n2, double cost)
{
	self->grid[self->n * n1 + n2] = cost;
}

static inline double get_network_cost(struct network *self, __u32 n1, __u32 n2)
{
	return self->grid[self->n * n1 + n2];
}

#endif	/* __NETWORK_H__ */


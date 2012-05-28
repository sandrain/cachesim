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
#ifndef	__PQUEUE_H__
#define	__PQUEUE_H__

#include <linux/types.h>
#include "cache_util.h"

struct pqueue {
	__u64 size;		/* size of the queue at a given time */
	__u64 capacity;		/* maximum size of the queue */

	int (*cmp) (const void *d1, const void *d2); /* compare function */

	struct cache_meta *data[0];	/* pointer to the data array */
};

#endif	/** __PQUEUE_H__ */


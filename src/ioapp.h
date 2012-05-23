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
#ifndef	__IOAPP_H__
#define	__IOAPP_H__

#include <stdio.h>
#include "cachesim.h"

enum {
	IOREQ_TYPE_EOF = 0,
	IOREQ_TYPE_READ,
	IOREQ_TYPE_WRITE,
	IOREQ_TYPE_ANY	/** for ARC trace files */
};

/**
 * io_request represents block io requests.
 */
struct io_request {
	__u32 node;		/* node id which generates this request */
	__u64 offset;		/* starting block offset */
	__u64 len;		/* number of blocks in this request */
	__u64 sequence;		/* the sequence of this request */
	int type;		/* read or write */
};

/** dump_io_request, for debugging. */
static inline
void dump_io_request(FILE *fp, struct io_request *req)
{
	fprintf(fp, "node %u: (%llu, %llu)\n",
			req->node, req->offset, req->len);
}

/**
 * io_app simulates an application which generates block io requests.
 */
struct ioapp {
	__u32 node;		/* node where this lives */
	FILE *trace;		/* input stream of trace file */

	char linebuf[64];	/* internally used buffer */
};

/**
 * ioapp_init initializes an ioapp instance. @self should be allocated by the
 * caller.
 *
 * @self: memory space to be used.
 * @node: node id for this application.
 * @filename: trace file path.
 *
 * returns newly initiated ioapp instance. NULL on failure.
 */
struct ioapp *ioapp_init(struct ioapp *self, __u32 node, char *filename);

/**
 * ioapp_exit de-initializes the ioapp instance.
 *
 * @self: ioapp instance.
 */
void ioapp_exit(struct ioapp *self);

/**
 * ioapp_next_request fetches block io requests one by one.
 *
 * @self: ioapp instance.
 * @req: [out] next io request.
 *
 * returns request type of fetched request (0, 1, or 2). <0 on error.
 */
int ioapp_next_request(struct ioapp *self, struct io_request *req);

#endif	/** __IOAPP_H__ */


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

struct io_request {
	__u32 node;
	__u64 offset;
	__u64 len;
	__u64 sequence;
	int type;
};

struct ioapp {
	__u32 node;
	FILE *trace;

	char linebuf[64];
};

struct ioapp *ioapp_init(struct ioapp *self, __u32 node, char *filename);

void ioapp_exit(struct ioapp *self);

int ioapp_next_request(struct ioapp *self, struct io_request *req);

#endif	/** __IOAPP_H__ */


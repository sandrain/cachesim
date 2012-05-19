#ifndef	__IOAPP_H__
#define	__IOAPP_H__

#include <stdio.h>
#include <linux/types.h>

struct io_request {

#define	IOREQ_TYPE_EOF		0
#define	IOREQ_TYPE_READ		1
#define	IOREQ_TYPE_WRITE	2

	int type;
	__u64 offset;
	__u64 len;
};

struct ioapp {
	__u32 node;
	FILE *trace;

	char linebuf[64];
};

struct ioapp *ioapp_init(__u32 node, char *filename);

void ioapp_exit(struct ioapp *self);

int ioapp_next_request(struct ioapp *self, struct io_request *req);

#endif	/** __IOAPP_H__ */


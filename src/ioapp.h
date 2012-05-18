#ifndef	__IOAPP_H__
#define	__IOAPP_H__

#include <stdio.h>
#include <linux/types.h>

struct io_request {
	__u64 offset;
	__u64 len;
};

struct ioapp {
	__u32 node;
	FILE *trace;
};

struct ioapp *ioapp_init(char *filename);

void ioapp_exit(struct ioapp *self);

struct io_request *ioapp_next_request(struct ioapp *self);

#endif	/** __IOAPP_H__ */


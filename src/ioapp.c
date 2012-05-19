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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ioapp.h"

extern int errno;

struct ioapp *ioapp_init(__u32 node, char *filename)
{
	struct ioapp *self = NULL;
	FILE *fp;

	fp = fopen(filename, "r");
	if (fp == NULL)
		return NULL;

	self = malloc(sizeof(*self));
	if (self) {
		self->node = node;
		self->trace = fp;
		memset(self->linebuf, 0, sizeof(self->linebuf));
	}
	else
		fclose(fp);

	return self;
}

void ioapp_exit(struct ioapp *self)
{
	if (self) {
		if (self->trace)
			fclose(self->trace);
		free(self);
	}
}

int ioapp_next_request(struct ioapp *self, struct io_request *req)
{
	char type;
	__u64 offset, len;

	if (!self || !req)
		return -EINVAL;

	do {
		char *line = fgets(self->linebuf, 63, self->trace);

		type = 0;

		if (line == NULL) {
			if (ferror(self->trace))
				return -errno;
			if (feof(self->trace))
				return IOREQ_TYPE_EOF;

			return -EBADF;
		}

		if (line[0] == '#')
			continue;

		if (sscanf(line, "%c %llu %llu", &type, &offset, &len) != 3)
			return -EINVAL;

		if (type != 'R' && type != 'W')
			return -EINVAL;

	} while (!type);

	req->type = type == 'R' ? IOREQ_TYPE_READ : IOREQ_TYPE_WRITE;
	req->offset = offset;
	req->len = len;

	return req->type;
}


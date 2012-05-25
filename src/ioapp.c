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
#include <ctype.h>

#include "ioapp.h"

struct ioapp *ioapp_init(struct ioapp *self, __u32 node, char *filename)
{
	FILE *fp;

	if (!self) {
		errno = EINVAL;
		return NULL;
	}

	fp = fopen(filename, "r");
	if (fp == NULL)
		return NULL;

	self->node = node;
	self->sequence = 0;
	self->trace = fp;
	memset(self->linebuf, 0, sizeof(self->linebuf));

	return self;
}

void ioapp_exit(struct ioapp *self)
{
	if (self) {
		if (self->trace)
			fclose(self->trace);
	}
}

int ioapp_next_request(struct ioapp *self, struct io_request *req)
{
	char type;
	__u64 offset, len, sequence;

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

		if (line[0] == '#' || isspace(line[0]))
			continue;

		if (sscanf(line, "%llu %llu %c %llu",
				&offset, &len, &type, &sequence) != 4)
			return -EINVAL;
	} while (!type);

	switch (type) {
	case 'R': req->type = IOREQ_TYPE_READ; break;
	case 'W': req->type = IOREQ_TYPE_WRITE; break;
	default: req->type = IOREQ_TYPE_ANY; break;
	}

	req->offset = offset;
	req->len = len;
	req->node = self->node;
	req->sequence = ++self->sequence;

	return req->type;
}


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

#include "cache.h"
#include "cache_util.h"

int cache_sync_block(struct local_cache *cache, __u64 block)
{
	struct io_request req;

	req.type = IOREQ_TYPE_WRITE;
	req.offset = block;
	req.len = 1;

	return local_cache_sync_block(cache, &req);
}

int cache_fetch_block(struct local_cache *cache, __u64 block)
{
	struct io_request req;

	req.type = IOREQ_TYPE_READ;
	req.offset = block;
	req.len = 1;

	return local_cache_fetch_block(cache, &req);
}

int cache_rw_cache_dev(struct local_cache *cache, __u64 block, int type)
{
	struct io_request req;

	req.type = type;
	req.offset = block;
	req.len = 1;

	return storage_rw_block(cache->local->ram, &req);
}

/**
 * The implementation of this hash table is based on the dr.v's
 * impelementation:
 *
 * hash table functions
 * copyright (c) 2005 dr. srinidhi varadarajan
 *
 * Just changed some coding styles.
 */

struct hash_table *hash_table_init(__u64 size)
{
	size_t memsize = 0;
	struct hash_table *self;

	memsize += sizeof(struct hash_table);
	memsize += sizeof(struct hash_entry *) * size * 2;

	self = (struct hash_table *) malloc(memsize);
	if (!self)
		return NULL;

	memset(self, 0, memsize);

	self->row = (struct hash_entry **) &self[1];
	self->tail = (struct hash_entry **) &self->row[size];
	self->size = size;

	return self;
}

void hash_table_exit(struct hash_table *self)
{
	__u64 i, size = self->size;
	struct hash_entry *current, *tmp;

	if (!self)
		return;

	for (i = 0; i < size; i++) {
		if (self->row[i]) {
			current = self->row[i];
			while (current) {
				tmp = current->next;
				free(current);
				current = tmp;
			}
			self->row[i] = NULL;
		}
	}

	free(self);
}

int hash_table_insert(struct hash_table *self,
			void *key, __u32 keylen, void *data)
{
	__u64 hash_key, size = self->size;
	size_t memsize = 0;
	struct hash_entry *entry, *prev;

	hash_key = hash(key, keylen, 7) % size;

	memsize += sizeof(struct hash_entry) + keylen;
	entry = (struct hash_entry *) malloc(memsize);
	if (!entry)
		return -ENOMEM;

	prev = self->tail[hash_key];
	entry->next = NULL;
	entry->prev = self->tail[hash_key];

	if (!prev)
		self->row[hash_key] = entry;
	else
		prev->next = entry;

	self->tail[hash_key] = entry;
	memcpy(entry->key, key, keylen);
	entry->data = data;
	entry->keylen = keylen;

	return 0;
}

int hash_table_delete(struct hash_table *self, void *key, __u32 keylen)
{
	uint32_t hash_key, size = self->size;
	struct hash_entry *current, *prev = NULL;

	hash_key  = hash(key, keylen, 7) % size;
	current = self->row[hash_key];

	while (current) {
		if (memcmp(current->key, key, keylen) == 0) {
			if (!prev) /* first entry */
				self->row[hash_key] = current->next;
			else
				prev->next = current->next;

			if (!current->next)
				self->tail[hash_key] = prev;

			free(current);

			return 0;
		}
		prev = current;
		current = current->next;
	}

	return -ENOENT;
}

void *hash_table_search(struct hash_table *self, void *key, __u32 keylen)
{
	__u64 hash_key, size = self->size;
	struct hash_entry *current;

	hash_key = hash(key, keylen, 7) % size;
	current = self->row[hash_key];

	while (current) {
		if (keylen == current->keylen
			&& memcmp(current->key, key, keylen) == 0)
		{
			return(current->data);
		}
		current = current->next;
	}

	return NULL;
}


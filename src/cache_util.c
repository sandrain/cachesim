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

/**
 * priority queue implementation. This implementation is based on the source
 * codes which can be found on:
 * http://andreinc.net/2011/06/01/implementing-a-generic-priority-queue-in-c/
 */

static inline __u64 parent_pos(__u64 pos)
{
	return (pos - 1) / 2;
}

static inline __u64 left_pos(__u64 pos)
{
	return pos * 2 + 1;
}

static inline __u64 right_pos(__u64 pos)
{
	return pos * 2 + 2;
}

static void pqueue_heapify(struct pqueue *self, __u64 pos)
{
	void *tmp = NULL;
	__u64 left, right, largest;

	left = left_pos(pos);
	right = right_pos(pos);

	if (left < self->size &&
		self->cmp(self->data[left], self->data[pos]) > 0)
	{
		largest = left;
	}
	else
		largest = pos;

	if (right < self->size &&
		self->cmp(self->data[right], self->data[largest]) > 0)
	{
		largest = right;
	}

	if (largest != pos) {
		tmp = self->data[largest];
		self->data[largest] = self->data[pos];
		self->data[pos] = tmp;

		pqueue_heapify(self, largest);
	}
}

struct pqueue *pqueue_init(__u64 capacity,
			int (*cmp) (const void *d1, const void *d2))
{
	struct pqueue *self = NULL;

	if (!cmp) {
		errno = EINVAL;
		return NULL;
	}

	self = malloc(sizeof(*self) + capacity * sizeof(void *));
	if (!self) {
		errno = ENOMEM;
		return NULL;
	}

	self->size = 0;
	self->capacity = capacity;
	self->cmp = cmp;

	return self;
}

void queue_exit(struct pqueue *self)
{
	if (self)
		free(self);
}

int pqueue_enqueue(struct pqueue *self, void *data)
{
	__u64 i;
	void *current = NULL;

	i = self->size;

	if (i >= self->capacity)
		return -ENOSPC;

	self->data[i] = data;
	self->size++;

	while (i > 0 &&
		self->cmp(self->data[i], self->data[parent_pos(i)]) > 0)
	{
		current = self->data[i];
		self->data[i] = self->data[parent_pos(i)];
		self->data[parent_pos(i)] = current;
		i = parent_pos(i);
	}

	return 0;
}

void *pqueue_dequeue(struct pqueue *self)
{
	void *data = NULL;

	if (self->size < 1)
		return NULL;

	data = self->data[0];
	self->data[0] = self->data[self->size - 1];
	self->size--;

	pqueue_heapify(self, 0);

	return data;
}


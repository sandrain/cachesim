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
#ifndef	__CACHE_UTIL_H__
#define	__CACHE_UTIL_H__

#include <linux/types.h>

#define	BLOCK_INVALID		((__u64) -1)

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

/** TODO:
 * Generic data structures for cache systems should be implemented. That
 * should include linked list, hash table, and binary min-heap..??
 *
 * - linked list implemented.
 */

enum { BLOCK_CLEAN = 0, BLOCK_DIRTY = 1 };

/**
 * This structure can be used to keep the general metadata for cached blocks.
 * Although this defines some common attributes, all of them can be freely used
 * by your algorithm.
 */
struct cache_meta {
	int dirty;		/* clean(0) or dirty(1)?? */
	__u64 index;		/* the index of the array */
	__u64 block;		/* physical block number */
	__u64 seq;		/* the request sequence */

	struct cache_meta *next;	/* list elements */
	struct cache_meta *prev;

	void *private;		/* any other data you want */
};

/**
 * init_cache_entry initializes all the fields in given cache_meta instance. If
 * you want to keep the members related to maintaining a list as they are, use
 * init_cache_entry_list() instead.
 *
 * @entry cache_meta instance.
 */
static inline
void init_cache_entry(struct cache_meta *entry)
{
	entry->index = BLOCK_INVALID;
	entry->dirty = BLOCK_CLEAN;
	entry->block = BLOCK_INVALID;
	entry->seq = 0;
	entry->next = NULL;
	entry->prev = NULL;
	entry->private = NULL;
}

/**
 * init_cache_entry_list works same as init_cache_entry but this keeps the list
 * related members as they are.
 *
 * @entry cache_meta instance.
 */
static inline
void init_cache_entry_list(struct cache_meta *entry)
{
	entry->index = BLOCK_INVALID;
	entry->dirty = BLOCK_CLEAN;
	entry->block = BLOCK_INVALID;
	entry->seq = 0;
	entry->private = NULL;
}

/**
 * generic_cache_entry_dump prints @binfo structure to the output stream
 * specified by @fp.
 *
 * @pos: the frame number.
 * @binfo: cache_meta instance.
 * @fp: output stream.
 */
static inline
void generic_cache_entry_dump(__u64 pos, struct cache_meta *binfo, FILE *fp)
{
	fprintf(fp, "[%5llu] %d, %llu, %llu\n",
			pos, binfo->dirty, binfo->block, binfo->seq);
}

/**
 * generic_cache_dump prints @binfo array to the output stream @fp.
 *
 * @binfo: the pointer to array of cache_meta instances.
 * @count: the number of instances in @binfo array
 * @fp: output stream.
 */
static inline
void generic_cache_dump(struct cache_meta *binfo, __u64 count, FILE *fp)
{
	__u64 i;

	for (i = 0; i < count; i++)
		generic_cache_entry_dump(i, &binfo[i], fp);
}

static inline
int cache_sync_block(struct local_cache *self, __u64 block)
{
	struct io_request tmp;

	tmp.type = IOREQ_TYPE_WRITE;
	tmp.offset = block;
	tmp.len = 1;

	return local_cache_sync_block(self, &tmp);
}

static inline
int cache_fetch_block(struct local_cache *self, __u64 block)
{
	struct io_request tmp;

	tmp.type = IOREQ_TYPE_READ;
	tmp.offset = block;
	tmp.len = 1;

	return local_cache_fetch_block(self, &tmp);
}

static inline
int cache_rw_cache_dev(struct local_cache *self, __u64 block, int type)
{
	struct io_request tmp;

	tmp.type = type;
	tmp.offset = block;
	tmp.len = 1;

	return storage_rw_block(self->local->ram, &tmp);
}

/**
 * cache_meta doubly linked list implementation. It's not a circular list.
 */

struct cache_meta_list {
	struct cache_meta *head;
	struct cache_meta *tail;

	__u64 size;	/* number of elements in the list */
};

/**
 * cache_meta_list_init initializes the list structure.
 *
 * @list: should be allocated by the caller.
 */
static inline
void cache_meta_list_init(struct cache_meta_list *list)
{
	if (list) {
		list->head = NULL;
		list->tail = NULL;
		list->size = 0;
	}
}

/**
 * cache_meta_list_insert_head inserts an element at the head of the given
 * list.
 *
 * @list: list instance.
 * @element: element to be inserted.
 */
static inline
void cache_meta_list_insert_head(struct cache_meta_list *list,
					struct cache_meta *element)
{
	if (!list)
		return;

	if (!list->head) {
		list->head = list->tail = element;
		element->next = element->prev = NULL;
	}
	else {
		element->prev = NULL;
		element->next = list->head;
		list->head->prev = element;
		list->head = element;
	}

	list->size++;
}

/**
 * cache_meta_list_insert_tail inserts an element at the tail of the given
 * list.
 *
 * @list: list instance.
 * @element: element to be inserted.
 */
static inline
void cache_meta_list_insert_tail(struct cache_meta_list *list,
					struct cache_meta *element)
{
	if (!list)
		return;

	if (!list->head) {
		list->head = list->tail = element;
		element->next = element->prev = NULL;
	}
	else {
		element->prev = list->tail;
		element->next = NULL;
		list->tail->next = element;
		list->tail = element;
	}

	list->size++;
}

/**
 * cache_meta_list_get_head passes the pointer to the element at the head of
 * the list. NOTE that this function doesn't modify the list structure. To
 * remove the head element, use cache_meta_list_remove_head() instead.
 *
 * @list: list instance.
 *
 * returns the element at the head of the list, NULL if the list is empty.
 */
static inline
struct cache_meta *cache_meta_list_get_head(struct cache_meta_list *list)
{
	return list ? list->head : NULL;
}

/**
 * cache_meta_list_get_tail passes the pointer to the element at the tail of
 * the list. NOTE that this function doesn't modify the list structure. To
 * remove the tail element, use cache_meta_list_remove_tail() instead.
 *
 * @list: list instance.
 *
 * returns the element at the tail of the list, NULL if the list is empty.
 */
static inline
struct cache_meta *cache_meta_list_get_tail(struct cache_meta_list *list)
{
	return list ? list->tail : NULL;
}

/**
 * cache_meta_list_remove_head removes the head element from the list and
 * returns it.
 *
 * @list: list instance
 *
 * returns the element at the head of the list, NULL if the list is empty.
 */
static inline
struct cache_meta *cache_meta_list_remove_head(struct cache_meta_list *list)
{
	struct cache_meta *res;

	if (!list || !list->head)
		return NULL;

	res = list->head;
	if (!res->next) {
		list->head = list->tail = NULL;
		list->size = 0;
		return res;
	}

	res->next->prev = NULL;
	list->head = res->next;

	res->next = res->prev = NULL;
	list->size--;

	return res;
}

/**
 * cache_meta_list_remove_tail removes the tail element from the list and
 * returns it.
 *
 * @list: list instance
 *
 * returns the element at the tail of the list, NULL if the list is empty.
 */
static inline
struct cache_meta *cache_meta_list_remove_tail(struct cache_meta_list *list)
{
	struct cache_meta *res;

	if (!list || !list->head)
		return NULL;

	res = list->tail;
	if (!res->prev) {
		list->head = list->tail = NULL;
		list->size = 0;
		return res;
	}

	res->prev->next = NULL;
	list->tail = res->prev;

	res->next = res->prev = NULL;
	list->size--;

	return res;
}

/**
 * cache_meta_list_remove removes element in an arbtrary position in the list.
 *
 * @list: list instance
 * @element: the pointer to the element which is to be removed.
 *
 * returns the pointer to the removed element, NULL on errors.
 */
static inline
struct cache_meta *cache_meta_list_remove(struct cache_meta_list *list,
					struct cache_meta *element)
{
	if (!list || !element)
		return NULL;

	if (list->head == element && list->tail == element) {
		list->head = list->tail = NULL;
		list->size = 0;

		return element;
	}

	if (!element->prev && !element->next)
		return NULL;	/* maybe not in the list */

	if (element->prev)
		element->prev->next = element->next;
	else
		list->head = element->next;

	if (element->next)
		element->next->prev = element->prev;
	else
		list->tail = element->prev;

	element->next = element->prev = NULL;
	list->size--;

	return element;
}

#endif	/** __CACHE_UTIL_H__ */


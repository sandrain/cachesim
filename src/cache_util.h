#ifndef	__CACHE_UTIL_H__
#define	__CACHE_UTIL_H__

#define	BLOCK_INVALID		((__u64) -1)

enum { BLOCK_CLEAN = 0, BLOCK_DIRTY = 1 };

struct cache_meta {
	int dirty;
	__u64 block;
	__u64 seq;
	void *private;
};

static inline
void init_cache_entry(struct cache_meta *entry)
{
	entry->dirty = BLOCK_CLEAN;
	entry->block = BLOCK_INVALID;
	entry->seq = 0;
	entry->private = NULL;
}

static inline
void generic_cache_dump(struct cache_meta *binfo, __u64 count, FILE *fp)
{
	__u64 i;

	for (i = 0; i < count; i++) {
		struct cache_meta *current = &binfo[i];
		fprintf(fp, "[%5llu] %d, %llu, %llu\n",
			i, current->dirty, current->block, current->seq);
	}
}

#endif	/** __CACHE_UTIL_H__ */


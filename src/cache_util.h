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

#endif	/** __CACHE_UTIL_H__ */


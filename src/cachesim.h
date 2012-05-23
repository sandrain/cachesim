#ifndef	__CACHESIM_H__
#define	__CACHESIM_H__

#include <stdio.h>
#include <errno.h>
#include <linux/types.h>

extern int errno;

struct node_statistics {
	__u32 id;

	__u64 ram_reads;
	__u64 ram_writes;
	__u64 ssd_reads;
	__u64 ssd_writes;
	__u64 hdd_reads;
	__u64 hdd_writes;

	__u64 *ram_wear;
	__u64 *ssd_wear;
	__u64 *hdd_wear;

	__u64 cache_hits;
	__u64 cache_misses;
	__u64 cache_replacements;
};

static inline
void print_statistics(FILE *fp, struct node_statistics *stat)
{
	fprintf(fp,	"\n[node %u]"
			"\nram_reads = %llu"
			"\nram_writes = %llu"
			"\nssd_reads = %llu"
			"\nssd_writes = %llu"
			"\nhdd_reads = %llu"
			"\nhdd_writes = %llu"
			"\ncache_hits = %llu"
			"\ncache_misses = %llu"
			"\ncache_replacements = %llu\n",
			stat->id,
			stat->ram_reads, stat->ram_writes,
			stat->ssd_reads, stat->ssd_writes,
			stat->hdd_reads, stat->hdd_writes,
			stat->cache_hits, stat->cache_misses,
			stat->cache_replacements);
}

#include "config.h"
#include "ioapp.h"
#include "storage.h"
#include "node.h"
#include "cache.h"

#endif	/** __CACHESIM_H__ */


#ifndef	__CACHESIM_H__
#define	__CACHESIM_H__

#include <linux/types.h>

#include "config.h"
#include "device.h"
#include "ioapp.h"
#include "cache.h"
#include "node.h"

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

#endif	/** __CACHESIM_H__ */


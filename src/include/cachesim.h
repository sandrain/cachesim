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
#ifndef	__CACHESIM_H__
#define	__CACHESIM_H__

#include <stdio.h>
#include <errno.h>
#include <linux/types.h>

extern int errno;

/**
 * This structure represents the statistics of a node.
 */
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

/**
 * print_statistics prints the statistic to the given output stream.
 *
 * @fp: the output stream.
 * @stat: node_statistics instance.
 */
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

/** FIXME: i don't like this style of inclusion */
#include "config.h"
#include "ioapp.h"
#include "storage.h"
#include "node.h"
#include "cache.h"

#endif	/** __CACHESIM_H__ */


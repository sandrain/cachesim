#ifndef	__CONFIG_H__
#define	__CONFIG_H__

#include <stdio.h>
#include <pthread.h>
#include <linux/types.h>

/**
 * cachesim_config holds the configurations of the simulation.
 */
struct cachesim_config {
	__u32 nodes;
	__u32 block_size;

	int ram_wear;
	int ssd_wear;
	int hdd_wear;

	int comnode_cache_policy;
	int pfsnode_cache_policy;

	__u64 comnode_ram_size;
	__u64 comnode_ssd_size;
	__u64 comnode_hdd_size;

	__u64 pfsnode_ram_size;
	__u64 pfsnode_ssd_size;
	__u64 pfsnode_hdd_size;

	__u64 ram_latency_read;
	__u64 ram_latency_write;
	__u64 ssd_latency_read;
	__u64 ssd_latency_write;
	__u64 hdd_latency_read;
	__u64 hdd_latency_write;

	__u64 *network_cost;
	__u64 *network_access;

	char *trace_file;
	FILE *output;
};

/**
 * The config structure is defined in cachesim.c and if you need to pull some
 * values you should use this pointer. e.g. you may want to get the block_size
 * for your cache implementation.
 */
extern struct cachesim_config *cachesim_config;

/**
 * get_network_cost returns the network cost between two nodes.
 *
 * @n1: node id
 * @n2: node id
 *
 * returns the network access cost.
 */
static inline __u64 get_network_cost(__u32 n1, __u32 n2)
{
	__u64 *grid = cachesim_config->network_cost;
	return grid[n1 * cachesim_config->nodes + n2];
}

/**
 * set_network_cost records that there has been an access from node @n1 to node
 * @n2 through the network. You can think that this is a counter.
 *
 * @n1: node id
 * @n2: node id
 */
static inline void set_network_access(__u32 n1, __u32 n2)
{
	__u64 *grid = cachesim_config->network_access;
	grid[n1 * cachesim_config->nodes + n2]++;
}

/**
 * Do NOT use this directly; use pfs_lock() and pfs_unlock() instead.
 */
extern pthread_mutex_t *pfs_mutex;

/**
 * pfs_lock and pfs_unlock synchronizes accesses to the pfs. This can be used
 * as a general simulation-wide lock. This function internally calls the
 * pthread_mutex_lock(), which means that this function can be blocked.
 */
static inline int pfs_lock(void)
{
	return pthread_mutex_lock(pfs_mutex);
}

static inline int pfs_unlock(void)
{
	return pthread_mutex_unlock(pfs_mutex);
}

#endif	/** __CONFIG_H__ */


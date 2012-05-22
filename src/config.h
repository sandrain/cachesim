#ifndef	__CONFIG_H__
#define	__CONFIG_H__

#include <stdio.h>
#include <pthread.h>
#include <linux/types.h>

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

extern struct cachesim_config *cachesim_config;

static inline __u64 get_network_cost(__u32 n1, __u32 n2)
{
	__u64 *grid = cachesim_config->network_cost;
	return grid[n1 * cachesim_config->nodes + n2];
}

static inline void set_network_access(__u32 n1, __u32 n2)
{
	__u64 *grid = cachesim_config->network_access;
	grid[n1 * cachesim_config->nodes + n2]++;
}

extern pthread_mutex_t *pfs_mutex;

static inline int pfs_lock(void)
{
	return pthread_mutex_lock(pfs_mutex);
}

static inline int pfs_unlock(void)
{
	return pthread_mutex_unlock(pfs_mutex);
}

#endif	/** __CONFIG_H__ */


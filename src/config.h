#ifndef	__CONFIG_H__
#define	__CONFIG_H__

#include <linux/types.h>

struct cachesim_config {
	__u32 nodes;
	__u32 block_size;

	int ram_wear;
	int ssd_wear;
	int hdd_wear;

	__u64 ram_latency_read;
	__u64 ram_latency_write;
	__u64 ssd_latency_read;
	__u64 ssd_latency_write;
	__u64 hdd_latency_read;
	__u64 hdd_latency_write;

	__u64 network_cost[0];
};

extern struct cachesim_config *cachesim_config;

static inline __u64 get_network_cost(__u32 n1, __u32 n2)
{
	__u64 *grid = cachesim_config->network_cost;
	return grid[n1 * cachesim_config->nodes + n2];
}

#endif	/** __CONFIG_H__ */


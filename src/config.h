#ifndef	__CONFIG_H__
#define	__CONFIG_H__

#include <linux/types.h>

struct cachesim_config {
	int ram_wear;
	int ssd_wear;
	int hdd_wear;

	__u64 ram_latency_read;
	__u64 ram_latency_write;
	__u64 ssd_latency_read;
	__u64 ssd_latency_write;
	__u64 hdd_latency_read;
	__u64 hdd_latency_write;
};

extern struct cachesim_config *cachesim_config;

#endif	/** __CONFIG_H__ */


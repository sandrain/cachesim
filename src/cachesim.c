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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <linux/types.h>

#include "cachesim.h"

#define MB	((__u64) 1 << 20)
#define GB	((__u64) 1 << 30)
#define TB	((__u64) 1 << 40)

/**
 * default config values
 */
static int _ram_wear = 0;
static int _ssd_wear = 1;
static int _hdd_wear = 0;
static __u32 _block_size = 512;
static __u64 _ram_latency_read = 0;
static __u64 _ram_latency_write = 0;
static __u64 _ssd_latency_read = 10;
static __u64 _ssd_latency_write = 20;
static __u64 _hdd_latency_read = 100;
static __u64 _hdd_latency_write = 100;

static __u32 _num_comnodes = 5;
static __u64 _comnode_ram_size = 2 * GB;
static __u64 _comnode_ssd_size = 0;
static __u64 _comnode_hdd_size = 0;

static __u64 _pfsnode_ram_size = 8 * GB;
static __u64 _pfsnode_ssd_size = 0;
static __u64 _pfsnode_hdd_size = 16 * TB;

static __u64 _netcost_local = 5;
static __u64 _netcost_pfs = 5;

static int _comnode_cache_policy = CACHE_POLICY_RANDOM;
static int _pfsnode_cache_policy = CACHE_POLICY_RANDOM;

static char *trace_file = "trace/OLTP.lis";

static struct cachesim_config __cachesim_config;
static pthread_mutex_t __pfs_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t barrier;

pthread_mutex_t *pfs_mutex = &__pfs_mutex;
struct cachesim_config *cachesim_config = &__cachesim_config;

/**
 * NOTE: About the simulation threads
 * The simulation runs num_comnodes threads, which means that no thread is
 * created for the pfs node. Only one pfs node can be simulated and it is
 * simulated by calling the pfs functions inside each comnode thread. The
 * sychronization is done by using pthread_mutex. Only a single pfs node
 * instance is globally defined and used.
 */
static pthread_t *threads;
static struct node **node_data;	/** holds the comnode data */
static struct node __pfs_node;	/** the only pfs node */
static struct node *pfs_node;
static char *memptr;

static struct cachesim_config *read_configuration(char *config_file)
{
	__u32 i;
	__u64 *netgrid;
	struct cachesim_config *config = &__cachesim_config;

	config->nodes = _num_comnodes;
	config->block_size = _block_size;
	config->ram_wear = _ram_wear;
	config->ssd_wear = _ssd_wear;
	config->hdd_wear = _hdd_wear;
	config->comnode_ram_size = _comnode_ram_size;
	config->comnode_ssd_size = _comnode_ssd_size;
	config->comnode_hdd_size = _comnode_hdd_size;
	config->pfsnode_ram_size = _pfsnode_ram_size;
	config->pfsnode_ssd_size = _pfsnode_ssd_size;
	config->pfsnode_hdd_size = _pfsnode_hdd_size;
	config->ram_latency_read = _ram_latency_read;
	config->ram_latency_write = _ram_latency_write;
	config->ssd_latency_read = _ssd_latency_read;
	config->ssd_latency_write = _ssd_latency_write;
	config->hdd_latency_read = _hdd_latency_read;
	config->hdd_latency_write = _hdd_latency_write;
	config->comnode_cache_policy = _comnode_cache_policy;
	config->pfsnode_cache_policy = _pfsnode_cache_policy;
	config->trace_file = strdup(trace_file);

	/** TODO: here should come the reading configuration file.. */

	netgrid = malloc(sizeof(__u64) * config->nodes * config->nodes * 2);
	if (!netgrid)
		return NULL;

	config->network_cost = netgrid;
	config->network_access = &netgrid[config->nodes * config->nodes];

	for (i = 0; i < config->nodes * config->nodes; i++) {
		config->network_cost[i] = _netcost_local;
		config->network_access[i] = 0;
	}

	for (i = 0; i < config->nodes; i++)
		config->network_cost[i] = _netcost_pfs;
	for (i = 0; i < config->nodes * config->nodes; i += config->nodes)
		config->network_cost[i] = _netcost_pfs;

	return config;
}

static int cachesim_prepare(struct cachesim_config *config)
{
	__u32 i, pos, node;
	int devcount, devcount_com, devcount_pfs;
	size_t memsize;
	struct storage *storage = NULL;
	struct local_cache *cache = NULL;
	struct ioapp *app = NULL;

	struct node *nodes = NULL;
	struct storage *ram = NULL;
	struct storage *ssd = NULL;
	struct storage *hdd = NULL;
	struct local_cache *lcache = NULL;
	struct ioapp *lapp = NULL;

	/** allocate memory space.. it's kind of painful :-( */
	devcount_com = devcount_pfs = 1;

	devcount_com += config->comnode_ssd_size ? 1 : 0;
	devcount_com += config->comnode_hdd_size ? 1 : 0;
	devcount_pfs += config->pfsnode_ssd_size ? 1 : 0;
	devcount_pfs += config->pfsnode_hdd_size ? 1 : 0;
	devcount = devcount_pfs + devcount_com * config->nodes;

	memsize = (sizeof(pthread_t) + sizeof(struct node))* config->nodes;
	memsize += sizeof(struct node *) * config->nodes;
	memsize += sizeof(struct local_cache) * (config->nodes + 1);
	memsize += sizeof(struct storage) * devcount;
	memsize += sizeof(struct ioapp) * config->nodes;

	if ((memptr = malloc(memsize)) == NULL)
		return -ENOMEM;

	threads = (pthread_t *) memptr;
	node_data = (struct node **) &threads[config->nodes];
	nodes = (struct node *) &node_data[config->nodes];
	cache = (struct local_cache *) &nodes[config->nodes];
	storage = (struct storage *) &cache[config->nodes + 1];
	app = (struct ioapp *) &storage[devcount];

	/** initialize computing nodes */
	for (i = 0, pos = 0; i < config->nodes; i++) {
		node = i + 1;
		ram = storage_init_ram(&storage[pos++], node,
				config->comnode_ram_size);
		if (!ram)
			return -errno;
		if (config->comnode_ssd_size) {
			ssd = storage_init_ssd(&storage[pos++], node,
						config->comnode_ssd_size);
			if (!ssd)
				return -errno;
		}
		if (config->comnode_hdd_size) {
			hdd = storage_init_hdd(&storage[pos++], node,
						config->comnode_hdd_size);
			if (!hdd)
				return -errno;
		}

		lapp = ioapp_init(&app[i], node, config->trace_file);
		if (!lapp)
			return -errno;

		node_data[i] = node_init_compute(&nodes[i], node, lapp,
						ram, ssd, hdd);
		if (!node_data[i])
			return -errno;

		lcache = local_cache_init(&cache[i], _comnode_cache_policy,
					node_data[i], &__pfs_node);
		node_set_local_cache(node_data[i], lcache);
	}

	/** initialize the pfs node */
	node = 0;
	ram = storage_init_ram(&storage[pos++], node,
				config->pfsnode_ram_size);
	if (!ram)
		return -errno;
	if (config->pfsnode_ssd_size) {
		ssd = storage_init_ssd(&storage[pos++], node,
				config->pfsnode_ssd_size);
		if (!ssd)
			return -errno;
	}
	if (config->pfsnode_hdd_size) {
		hdd = storage_init_hdd(&storage[pos++], node,
				config->pfsnode_hdd_size);
		if (!hdd)
			return -errno;
	}

	pfs_node = node_init_pfs(&__pfs_node, ram, ssd, hdd);
	if (!pfs_node)
		return -errno;

	lcache = local_cache_init(&cache[i], _pfsnode_cache_policy,
				pfs_node, NULL);
	node_set_local_cache(pfs_node, lcache);

	return 0;


	/** TODO: this initialization process should be re-written!! the order
	 * of allocation is not correct; the cleanup() function cannot clean
	 * the garbages in case of failure. well, it's not a big deal since
	 * anyway we terminate the program in case of failure of this function.
	 * but it should be rewritten!!
	 * i personally prefer the above comment-outed method to the frequent
	 * malloc(). then the interfaces of all modules also should be
	 * rewritten!
	 */
#if 0
	__u32 i;
	__u64 block_size;
	struct node_data *cnode;
	struct ioapp *capp;
	struct storage *cram, *cssd, *cdisk;
	struct cache *ccache;
	struct storage *devs[3];

	node_data = malloc(sizeof(struct node *) * config->nodes);
	if (!node_data)
		return -ENOMEM;

	/** initialize the computing nodes */
	for (i = 0; i < config->nodes; i++) {
		memset(node_data, 0, sizeof(struct node *) * config->nodes);

		block_size = config->block_size;

		/** initialize pfs node first */
		cram = storage_init_ram(i, block_size,
					config->pfsnode_ram_size / block_size);
		if (!cram)
			return -ENOMEM;
		if (config->pfsnode_ssd_size) {
			cssd = storage_init_ssd(i, block_size,
					config->pfsnode_ssd_size / block_size);
			if (!cssd)
				return -ENOMEM;
		}
		if (config->pfsnode_hdd_size) {
			chdd = storage_init_hdd(i, block_size,
					config->pfsnode_hdd_size / block_size);
			if (!chdd)
				return -ENOMEM;
		}

		devs[CACHEDEV_RAM] = cram;
		devs[CACHEDEV_SSD] = cssd;
		devs[CACHEDEV_HDD] = chdd;

		ccache = local_cache_init(0, CACHE_POLICY_RANDOM, 3, devs);

		capp = ioapp_init(i, config->trace_file);
		if (!capp)
			return -ENOMEM;

		cnode = node_init_compute(i, capp, ccache, cram, cssd, chdd);
		if (!cnode)
			return -ENOMEM;
		node_data[i] = cnode;
	}

	return 0;
#endif
}

void *thread_main(void *arg)
{
	int res;
	struct node_statistics stat;
	struct node *node = (struct node *) arg;

	res = node_service_ioapp(node);

	node_get_statistics(node, &stat);
	pthread_barrier_wait(&barrier);

	pfs_lock();
	print_statistics(stdout, &stat);
	pfs_unlock();

	return (void *) 0;
}

static void cleanup(void)
{
	__u32 i;

	if (node_data) {
		for (i = 0; i < cachesim_config->nodes; i++) {
			struct node *current = node_data[i];
			if (current) {
				if (current->app)
					ioapp_exit(current->app);
				if (current->cache)
					local_cache_exit(current->cache);
				if (current->ram)
					storage_exit(current->ram);
				if (current->ssd)
					storage_exit(current->ssd);
				if (current->hdd)
					storage_exit(current->hdd);
				if (current->private)
					free(current->private);
			}
			node_exit(current);
		}
		free(memptr);
	}

	if (cachesim_config->network_cost)
		free(cachesim_config->network_cost);
}

static void print_usage(const char *exe)
{
	printf(	"\nUsage: %s <configuration file>\n\n", exe);
}

/**
 * The main program.
 */
int main(int argc, char **argv)
{
	int res = 0;
	__u32 i;

	if (argc != 2) {
		print_usage(argv[0]);
		return 0;
	}

	cachesim_config = read_configuration(argv[1]);
	if (!cachesim_config) {
		perror("Failed to read configuration");
		res = errno;
		goto out;
	}

	if ((res = cachesim_prepare(cachesim_config)) < 0) {
		perror("Failed to setup the simulation");
		goto out;
	}

	pthread_barrier_init(&barrier, NULL, cachesim_config->nodes);

	for (i = 0; i < cachesim_config->nodes; i++) {
		if (pthread_create(&threads[i], NULL,
					thread_main, node_data[i])) {
			i++;
			goto out_thread;
		}
	}

out_thread:
	while (i--)
		pthread_join(threads[i], (void **) NULL);

	pthread_barrier_destroy(&barrier);

	if (!res) {	/** print the statistics of the pfs node */
		struct node_statistics stat;

		node_get_statistics(pfs_node, &stat);
		print_statistics(stdout, &stat);
	}
out:
	cleanup();
	return res;
}


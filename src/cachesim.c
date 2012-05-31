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
#include <ctype.h>
#include <getopt.h>
#include <pthread.h>
#include <linux/types.h>

#include "cachesim.h"

#define	KB	((__u64) 1 << 10)
#define MB	((__u64) 1 << 20)
#define GB	((__u64) 1 << 30)
#define TB	((__u64) 1 << 40)

#define	DEFAULT_TRACE_FILE	"trace/OLTP.lis"

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
static __u64 _pfsnode_hdd_size = 1;

static __u64 _netcost_local = 5;
static __u64 _netcost_pfs = 5;

static int _comnode_cache_policy = CACHE_POLICY_RANDOM;
static int _pfsnode_cache_policy = CACHE_POLICY_RANDOM;

static char *_comnode_cache_policy_str;
static char *_pfsnode_cache_policy_str;

static char *_trace_file;

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

enum { OPT_STR = 0, OPT_U32, OPT_U64 };

struct config_options {
	char *opstr;
	int optype;
	void *opval;
};

static struct config_options options[] = {
	{ "ram_wear", OPT_U32, &_ram_wear },
	{ "ssd_wear", OPT_U32, &_ssd_wear },
	{ "hdd_wear", OPT_U32, &_hdd_wear },
	{ "trace_file", OPT_STR, &_trace_file },
	{ "block_size", OPT_U64, &_block_size },
	{ "ram_latency_read", OPT_U64, &_ram_latency_read },
	{ "ram_latency_write", OPT_U64, &_ram_latency_write },
	{ "ssd_latency_read", OPT_U64, &_ssd_latency_read },
	{ "ssd_latency_write", OPT_U64, &_ssd_latency_write },
	{ "hdd_latency_read", OPT_U64, &_hdd_latency_read },
	{ "hdd_latency_write", OPT_U64, &_hdd_latency_write },
	{ "num_comnodes", OPT_U64, &_num_comnodes },
	{ "comnode_ram_size", OPT_U64, &_comnode_ram_size },
	{ "comnode_ssd_size", OPT_U64, &_comnode_ssd_size },
	{ "comnode_hdd_size", OPT_U64, &_comnode_hdd_size },
	{ "pfsnode_ram_size", OPT_U64, &_pfsnode_ram_size },
	{ "pfsnode_ssd_size", OPT_U64, &_pfsnode_ssd_size },
	{ "pfsnode_hdd_size", OPT_U64, &_pfsnode_hdd_size },
	{ "comnode_cache_policy", OPT_STR, &_comnode_cache_policy_str },
	{ "pfsnode_cache_policy", OPT_STR, &_pfsnode_cache_policy_str },
	{ "netcost_local", OPT_U64, &_netcost_local },
	{ "netcost_pfs", OPT_U64, &_netcost_pfs }
};

static int option_len = sizeof(options) / sizeof(struct config_options);
static char linebuf[256];

static char *config_file;
static char *output_file;

static char *cache_policies[] = {
	"none", "random", "opt", "fifo", "lru", "mru", "lfu", "arc",
	"lirs", "2q"
};

/**
 * read_configuration: reads and parses the configuration file.
 *
 * @config_file: the path to the configuration file.
 *
 * returns the cachesim_config, which is filled.
 */
static struct cachesim_config *read_configuration(char *config_file)
{
	__u32 i, lcount = 0;
	__u64 *netgrid;
	FILE *fp;
	char *token;
	struct cachesim_config *config = &__cachesim_config;

	/** read from the given configuration file. */
	if ((fp = fopen(config_file, "r")) == NULL) {
		perror("Failed to open the configuration file");
		return NULL;
	}

	while (fgets(linebuf, 255, fp) != NULL) {
		lcount++;

		if (linebuf[0] == '#' || isspace(linebuf[0]))
			continue;

		token = strtok(linebuf, " \t\n=");

		for (i = 0; i < option_len; i++) {
			if (!strcmp(options[i].opstr, token)) {
				struct config_options *current = &options[i];
				void *val = current->opval;

				token = strtok(NULL, " \t\n=");
				if (!token) {
					fprintf(stderr,
						"Failed to parse line: %u\n",
						lcount);
					config = NULL;
					goto out;
				}

				switch (current->optype) {
				case OPT_STR:
					*((char **) val) = strdup(token);
					break;
				case OPT_U32:
					*((__u32 *) val) = (__u32) atol(token);
					break;
				case OPT_U64:
					*((__u64 *) val) = (__u64) atoll(token);
					break;
				default:
					fprintf(stderr,
						"Failed to parse line: %u\n",
						lcount);
					config = NULL;
					goto out;
				}

				break;
			}
		}

		if (i == option_len) {
			fprintf(stderr, "Unknown option %s at line %d\n",
					token, lcount);
			config = NULL;
			goto out;
		}
	}
	if (ferror(fp)) {
		perror("Error while reading the configuration file");
		config = NULL;
		goto out;
	}

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
	config->trace_file = _trace_file;

	for (i = 0; i < sizeof(cache_policies) / sizeof(char *); i++) {
		if (!strcmp(_comnode_cache_policy_str, cache_policies[i]))
			config->comnode_cache_policy = i;
		if (!strcmp(_pfsnode_cache_policy_str, cache_policies[i]))
			config->pfsnode_cache_policy = i;
	}

	if (!config->trace_file)
		config->trace_file = DEFAULT_TRACE_FILE;

	netgrid = malloc(sizeof(__u64) * config->nodes * config->nodes * 2);
	if (!netgrid) {
		perror("Failed to allocate memory");
		config = NULL;
		goto out;
	}

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

out:
	fclose(fp);
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

		lcache = local_cache_init(&cache[i],
					config->comnode_cache_policy,
					node_data[i],
					&__pfs_node);
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

	lcache = local_cache_init(&cache[i], config->pfsnode_cache_policy,
				pfs_node, NULL);
	node_set_local_cache(pfs_node, lcache);

	return 0;
}

static void print_node_result(struct node *node)
{
	struct node_statistics stat;

	node_get_statistics(node, &stat);

	pfs_lock();

	print_statistics(cachesim_config->output, &stat);
	fputs("cache status --------------------------------\n",
		cachesim_config->output);
	local_cache_dump(node->cache, cachesim_config->output);
	fputs("---------------------------------------------\n",
		cachesim_config->output);

	pfs_unlock();
}

void *thread_main(void *arg)
{
	int res = 0;
	struct node *node = (struct node *) arg;

	res = node_service_ioapp(node);
	if (res) {
		/** node_service_ioapp always returns 0 at this moment. */
	}

	pthread_barrier_wait(&barrier);
	print_node_result(node);

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

	if (cachesim_config) {
		if (cachesim_config->output)
			fclose(cachesim_config->output);
		if (cachesim_config->network_cost)
			free(cachesim_config->network_cost);
	}
}

static const char *usage_string = 
"\nUsage: cachesim [-o output_file] [-f config_file]\n"
"\n  If you don't specify any options, cachesim will use cachesim.conf"
"\n  as a default configuration file and print the output to stdout.\n\n";

static inline void print_usage(const char *exe)
{
	fputs(usage_string, stderr);
}

/**
 * The main program.
 */
int main(int argc, char **argv)
{
	int opt;
	int res = 0;
	__u32 i;
	FILE *outfp = NULL;

	while ((opt = getopt(argc, argv, "ho:f:")) != -1) {
		switch (opt) {
		case 'o': output_file = optarg; break;
		case 'f': config_file = optarg; break;
		case 'h':
		default: print_usage(argv[0]);
			 return 0;
		}
	}

	if (!config_file)
		config_file = "cachesim.conf";

	if (output_file) {
		outfp = fopen(output_file, "w");
		if (!outfp) {
			perror("Failed to open the output file");
			return -errno;
		}
	}
	else
		outfp = stdout;

	cachesim_config = read_configuration(config_file);
	if (!cachesim_config) {
		res = errno;
		goto out;
	}

	cachesim_config->output = outfp;

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

	if (!res)	/** print the statistics of the pfs node */
		print_node_result(pfs_node);
out:
	cleanup();
	return res;
}


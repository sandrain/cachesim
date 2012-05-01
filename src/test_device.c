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
#include <assert.h>
#include <linux/types.h>

#include "device.h"

struct network {
	double cost;
};

double network_get_cost(struct network *network, __u32 n1, __u32 n2)
{
	return network->cost;
}

static double latency[3][2] = {
	{0.1, 0.2}, {10.0, 20.0}, {70.0, 80.0}
};

static __u32 blksize = 4096;

static char *private_str;
struct network net = { 10.0 };

int main(void)
{
	int i, res;
	double tmp;
	struct stdev *dev[3];
	struct stdev_stat stat;

	dev[0] = stdev_mem_init(latency[0][0], latency[0][1],
				blksize, 4194304);	/* 16GB */
	assert(dev[0] != NULL);
	dev[1] = stdev_hdd_init(latency[1][0], latency[1][1],
				blksize, 53687091);	/* 2TB */
	assert(dev[1] != NULL);
	dev[2] = stdev_ssd_init(latency[2][0], latency[2][1],
				blksize, 33554432, 262144); /* 128GB, 256K */
	assert(dev[2] != NULL);

	for (i = 0; i < 3; i++) {
		assert(dev[0]->ops->get_read_latency);
		assert(dev[0]->ops->get_write_latency);
		assert(dev[0]->ops->get_stat);
		assert(dev[0]->ops->read_block);
		assert(dev[0]->ops->write_block);

		tmp = dev[i]->ops->get_read_latency(dev[i]);
		assert(tmp == latency[i][0]);

		tmp = dev[i]->ops->get_write_latency(dev[i]);
		assert(tmp == latency[i][1]);

		res = dev[i]->ops->read_block(dev[i], 0, 0, 10, &tmp);
		assert(tmp == latency[i][0]);

		res = dev[i]->ops->write_block(dev[i], 0, 0, 10, &tmp);
		assert(tmp == latency[i][1]);

		res = dev[i]->ops->get_stat(dev[i], &stat);
		assert(stat.read_count == 1);
		assert(stat.write_count == 1);
		assert(stat.read_blocks = 10);
		assert(stat.written_blocks = 10);

		res = dev[i]->ops->read_block(dev[i], 0, 2, 5, &tmp);
		assert(tmp == latency[i][0]);
		res = dev[i]->ops->write_block(dev[i], 0, 10, 3, &tmp);
		assert(tmp == latency[i][1]);

		res = dev[i]->ops->get_stat(dev[i], &stat);
		assert(stat.read_count == 2);
		assert(stat.write_count == 2);
		assert(stat.read_blocks = 5);
		assert(stat.written_blocks = 3);

		/* network test */
		stdev_set_node(dev[i], 0);
		stdev_set_network(dev[i], &net);

		res = dev[i]->ops->read_block(dev[i], 1, 2, 3, &tmp);
		assert(tmp == latency[i][0] + net.cost);

		res = dev[i]->ops->write_block(dev[i], 1, 2, 3, &tmp);
		assert(tmp == latency[i][1] + net.cost);

		/* private test */
		private_str = malloc(sizeof("hello"));
		strncpy(private_str, "hello", strlen("hello"));
		stdev_set_private(dev[i], private_str);
		assert((char *) dev[i]->private == private_str);
	}

	free(private_str);

	for (i = 0; i < 3; i++)
		stdev_exit(dev[i]);

	printf("All test passed!\n");

	return 0;
}


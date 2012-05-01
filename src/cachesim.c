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

#include "cachesim.h"

static int read_config_file(struct cachesim_config *config, char *config_file)
{
	return 0;
}

static struct cachesim *build_cachesim(struct cachesim_config *config)
{
	return NULL;
}

static int cachesim_run(struct cachesim *sim)
{
	return 0;
}

static void cachesim_report(struct cachesim *sim)
{
}

static void print_usage(char *exe)
{
	fprintf(stderr, "Usage: %s <config file>\n", exe);
}

int main(int argc, char **argv)
{
	int res = 0;
	char *config_file;
	struct cachesim *sim;
	struct cachesim_config config;

	if (argc != 2) {
		print_usage(argv[0]);
		return 0;
	}
	config_file = argv[1];

	if (read_config_file(&config, config_file) < 0)
		return -1;

	sim = build_cachesim(&config);
	if (!sim)
		return -2;

	if ((res = cachesim_run(sim)) < 0)
		fprintf(stderr, "Simulation failed: %d\n", res);
	else
		cachesim_report(sim);

	free(sim);

	return 0;
}


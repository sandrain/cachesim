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
#include <pthread.h>
#include <linux/types.h>

#include "cachesim.h"

/**
 * default config values
 */
static int __ram_wear = 0;
static int __ssd_wear = 1;
static int __hdd_wear = 0;
static __u32 __block_size = 512;
static __u64 __ram_latency_read = 0;
static __u64 __ram_latency_write = 0;
static __u64 __ssd_latency_read = 10;
static __u64 __ssd_latency_write = 20;
static __u64 __hdd_latency_read = 100;
static __u64 __hdd_latency_write = 100;

static __u64 __netcost_local = 5;
static __u64 __netcost_pfs = 5;

static struct cachesim_config __cachesim_config;
static pthread_mutex_t __pfs_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv)
{
}


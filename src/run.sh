#!/bin/bash

# LRU
for test_size in 1000 2000 5000 10000 15000
do
	sed "/^comnode_cache_policy = */ c comnode_cache_policy = lru" \
	    < cachesim.conf.in | \
	    sed "/^comnode_ram_size = */ c comnode_ram_size = $test_size" \
	    > cachesim.conf
	./cachesim > io/postmark/lru_${test_size} \
		  2> io/postmark/lru_${test_size}_io
done

# ARC
for test_size in 1000 2000 5000 10000 15000
do
	sed "/^comnode_cache_policy = */ c comnode_cache_policy = arc" \
	    < cachesim.conf.in | \
	    sed "/^comnode_ram_size = */ c comnode_ram_size = $test_size" \
	    > cachesim.conf
	./cachesim > io/postmark/arc_${test_size} \
		  2> io/postmark/arc_${test_size}_io
done


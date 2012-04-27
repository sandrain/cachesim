# Copyright (C) Hyogi Sim <hyogi@cs.vt.edu>
# 
# ---------------------------------------------------------------------------
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# 

import array

class StorageDevice(object):
    def __init__(self, capacity, block_size, read_latency, write_latency):
        self.__capacity = capacity
        self.__block_size = block_size
        self.__read_latency = read_latency
        self.__write_latency = write_latency
        self.__read_count = 0
        self.__write_count = 0

    @property
    def read_latency(self):
        return self.__read_latency

    @property
    def write_latency(self):
        return self.__write_latency

    @property
    def capacity(self):
        return self.__capacity

    @property
    def block_size(self):
        return self.__block_size

    @property
    def read_count(self):
        return self.__read_count

    @property
    def write_count(self):
        return self.__write_count

    def read_block(self, block, count):
        self.__read_count += 1
        return self.read_latency;

    def write_block(self, block, count):
        self.__write_count += 1
        return self.write_latency;

class Memory(StorageDevice):
    def __init__(self, capacity, block_size, \
                read_latency=0, write_latency=0):
        StorageDevice.__init__(self, capacity, block_size, read_latency, \
                            write_latency)

class SSD(StorageDevice):
    """ In SSD class, we keep track of write distribution among blocks.
    """
    def __init__(self, capacity, block_size, read_latency, write_latency):
        StorageDevice.__init__(self, capacity, block_size, read_latency, \
                            write_latency)

        # if this overflows, we need to use c_int64
        self.block_wear = array.array('L', map(lambda x:0, range(capacity)))

    def write_block(self, block, count):
        self.__write_count += 1
        for i in range(block, block + count):
            self.block_wear[i] += 1
        return self.__write_latency

class HDD(StorageDevice):
    def __init__(self, capacity, block_size, \
                read_latency=100.0, write_latency=100.0):
        StorageDevice.__init__(self, capacity, block_size, read_latency, \
                            write_latency)

# Test driver
if __name__ == '__main__':
    pass


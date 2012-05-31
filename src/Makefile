#
# Makefile
#

CC	= gcc
CFLAGS	= -ggdb -O0 -Wall -Iinclude
#LDFLAGS	= -lpthread
LDFLAGS	= -pthread

TARGET	= cachesim
OBJS	= cachesim.o ioapp.o node.o storage.o cache.o cache_random.o \
	  cache_util.o cache_opt.o cache_2q.o \
	  cache_fifo.o cache_lru.o cache_lfu.o cache_arc.o cache_lirs.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(TARGET)

clean:
	rm -f *.o $(TARGET)


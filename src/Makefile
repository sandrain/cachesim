#
# Makefile
#

CC	= gcc
CFLAGS	= -ggdb -O0 -Wall
LDFLAGS	= -lpthread

TARGET	= cachesim
OBJS	= cachesim.o ioapp.o node.o storage.o cache.o cache_random.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(TARGET)

clean:
	rm -f *.o $(TARGET)


#
# Makefile
#

CC	= gcc
CFLAGS	= -ggdb -O0
LDFALGS	= -lpthread

TARGET	= cachesim
OBJS	= cachesim.o cache.o ioapp.o node.o storage.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(TARGET)

clean:
	rm -f *.o $(TARGET)


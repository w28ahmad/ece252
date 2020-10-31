CC = gcc      					# compiler
CFLAGS = -Wall -g -std=gnu99	# compilation flg 
LD = gcc      					# linker
LDFLAGS = -g -std=gnu99   		# debugging symbols in build
LDLIBS  = -lcurl -lpthread -lz -lm	# link with libpthread, lm for ceil
LIB_UTIL = zutil.o crc.o lab_png.o

SRCS = paster2.c
OBJS1  = paster2.o
OBJS2 = catpng.o $(LIB_UTIL)
TARGETS = paster2 catpng

all: ${TARGETS}

paster2: ${OBJS1}
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

catpng: $(OBJS2)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o *~ $(TARGETS) *.png
	# Clear shared memory
	chmod +x clean_ipcs.sh
	./clean_ipcs.sh

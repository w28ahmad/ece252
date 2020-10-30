CC = gcc      					# compiler
CFLAGS = -Wall -g -std=gnu99	# compilation flg 
LD = gcc      					# linker
LDFLAGS = -g -std=gnu99   		# debugging symbols in build
LDLIBS  = -lcurl -lpthread -lm	# link with libpthread, lm for ceil

SRCS = paster2.c
OBJS  = paster2.o
TARGETS = paster2

all: ${TARGETS}
paster2: ${OBJS}
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
	./clean_ipcs.sh


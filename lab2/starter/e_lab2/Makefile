CC = gcc 
CFLAGS = -Wall -std=gnu99 -g # "curl-config --cflags" output is empty  
LD = gcc
LDFLAGS = -std=c99 -g 
LDLIBS = -lcurl -lz # lcurl "curl-config --libs" output, lz link with libz

LIB_UTIL = zutil.o crc.o lab_png.o
SRCS   = paster.c crc.c zutil.c lab_png.c
OBJS1  = paster.o
OBJS2  = catpng.o $(LIB_UTIL)
TARGETS= paster.out catpng.out 

all: ${TARGETS}

paster.out: $(OBJS1)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

catpng.out: $(OBJS2)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	 gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) *.png

# Makefile, ECE252 
# Yiqing Huang

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o lab_png.o
SRCS   = crc.c zutil.c lab_png.c
OBJS	= catpng.o $(LIB_UTIL)

# ADD THE NEEDED FUNCTIONS HERE
TARGETS= catpng.out

all: ${TARGETS}

# ADD THE TARGETS HERE
catpng.out: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 


%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

# ADD THE CLEANS HERE
.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) all.png 

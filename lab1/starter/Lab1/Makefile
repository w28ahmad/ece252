# Makefile, ECE252 
# Yiqing Huang


CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o
SRC1   = pnginfo.c crc.c
OBJ1   = pnginfo.o $(LIB_UTIL) 

SRC2   = findpng.c crc.c
OBJ2   = findpng.o $(LIB_UTIL)

SRC3   = crc.c zutil.c lab_png.c catpng.c
OBJ3   = catpng.o lab_png.o $(LIB_UTIL)

NAME1 = pnginfo
NAME2 = findpng
NAME3 = catpng
PNG_OUT = all.png

all: $(NAME1) $(NAME2) $(NAME3)

$(NAME1): $(OBJ1)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(NAME2): $(OBJ2)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(NAME3): $(OBJ3)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<  

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(NAME1) $(NAME2) $(NAME3) $(PNG_OUT)

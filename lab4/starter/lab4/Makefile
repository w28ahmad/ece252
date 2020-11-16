CC = gcc 
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)
CFLAGS = -Wall $(CFLAGS_XML2) $(CFLAGS_CURL) -std=gnu99 -g -DDEBUG1_
LD = gcc
LDFLAGS = -std=gnu99 -g 
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) -lpthread 

SRCS   = findpng.c
OBJS3  = findpng.o
TARGETS= findpng2

all: ${TARGETS}

findpng2: $(OBJS3) 
		$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
		$(CC) $(CFLAGS) -c $< 

%.d: %.c
		gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

# Sample test
test1:
	# Note: 
	# if -m is too high, the hashtable might not behave properly
	# It seems to be very sensitive to data collisions
	# "cannot allocate memory" output implies hashtable is not large enough
	./findpng2 -t 10 -m 20 -v "log.txt" http://ece252-1.uwaterloo.ca/lab4 > output.txt
	# if the follow 2 commands output any lines something is wrong
	# Since the output will indicades duplicate links in a file
	# TODO: log.txt sometimes has a duplicate. Have not found a fix yet.
	sort log.txt  | uniq -d
	sort png_urls.txt  | uniq -d

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) *.png *.html *.txt


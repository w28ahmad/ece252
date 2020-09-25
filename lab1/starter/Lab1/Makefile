all: pnginfo
pnginfo: pnginfo.c lab_png.h
	gcc -o pnginfo pnginfo.c
pnginfo.o: pnginfo.c
	gcc -c pnginfo.c
clean:
	rm *.o pnginfo
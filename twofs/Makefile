COMPILER = gcc
FILESYSTEM_FILES = twofs.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) $(FILESYSTEM_FILES) -o twofs `pkg-config fuse --cflags --libs`

clean:
	rm twofs
	rm *~

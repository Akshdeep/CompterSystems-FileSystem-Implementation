#
# file:        Makefile - programming assignment 3
#

CFLAGS = -D_FILE_OFFSET_BITS=64 -g `pkg-config --cflags fuse`
LD_LIBS = `pkg-config --libs fuse`
ifdef COVERAGE
CFLAGS += -fprofile-arcs -ftest-coverage
LD_LIBS += --coverage
endif

FILE = homework
TOOLS = mktest read-img mkfs-x6

# note that implicit make rules work fine for compiling x.c -> x
# (e.g. for mktest). Also, the first target defined in the file gets
# compiled if you run without an argument.
#
all: homework $(TOOLS)

%.o: %.c
	${CC} ${CFLAGS} $^ -c -o $@

# '$^' expands to all the dependencies (i.e. misc.o homework.o image.o)
# and $@ expands to 'homework' (i.e. the target)
#
homework: misc.o homework.o image.o
	gcc -g $^ -o $@ $(LD_LIBS)

clean: 
	rm -f *.o homework $(TOOLS)

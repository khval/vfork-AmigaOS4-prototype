
CFLAGS = -D__USE_INLINE__

all:	vfork_imp.o
	gcc $(CFLAGS) -o vfork_demo vfork_demo.c vfork_imp.o


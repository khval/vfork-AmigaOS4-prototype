
CFLAGS = -D__USE_INLINE__ 
CFLAGS += -Ddebug=0

all:	vfork_imp.o debug.o
	gcc $(CFLAGS) -o vfork_demo vfork_demo.c vfork_imp.o debug.o

clean:
	delete #?.o vfork_demo
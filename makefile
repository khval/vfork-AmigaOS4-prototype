
CFLAGS = -D__USE_INLINE__ 
CFLAGS += -Ddebug=1

demos += vfork_demo.elf
demos += vfork_demo2.elf
demos += vfork_demo3.elf

all:	vfork_imp.o debug.o $(demos)

%.elf:  %.c vfork_imp.o debug.o
	gcc $(CFLAGS)  $< vfork_imp.o debug.o -o $@

clean:
	delete #?.o #?.elf
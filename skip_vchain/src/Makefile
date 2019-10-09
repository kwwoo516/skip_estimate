
CC=gcc
LDLIBS= -lpthread -lm -g

all: skip 
#cskip opskip

skip: main.c skip.c sim.h skip.h 
	$(CC) -o skip main.c skip.c $(LDLIBS) 
opskip: main.c opskip.c sim.h skip.h
	$(CC) -o opskip main.c skip.c $(LDLIBS) 
cskip: main.c cskip.c sim.h skip.h
	$(CC) -o cskip main.c cskip.c $(LDLIBS) -DCSKIP_LIST 
#vcskip: main.c vcskip.c rusage.c sim.h skip.h rusage.h
#	$(CC) -o vcskip main.c vcskip.c rusage.c $(LDLIBS) -DCSKIP_LIST -DVALUE_CHAIN
clean:
	rm -rf *.o skip pskip opskip *.gch cskip
	#rm -rf *.o skip pskip opskip *.gch cskip vcskip


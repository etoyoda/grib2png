OBJS=gribscan.o mainlogic.o visual.o mymalloc.o
OBJS2=gribscan.o mainslim.o mymalloc.o filter.o
OBJS3=gribscan.o mainpick.o mymalloc.o filter.o
LIBS= -lm -lpng
# OPTS= -g -pg を想定
LFLAGS= $(OPTS) -fopenmp
CFLAGS= $(OPTS) -fopenmp -O2 --pedantic -std=gnu99 -Wall 
CC=cc

.SUFFIXES:
.SUFFIXES: .c .o

all: grib2png gribslim gribpick

grib2png: $(OBJS)
	$(CC) $(LFLAGS) -o grib2png $(OBJS) $(LIBS)

gribslim: $(OBJS2)
	$(CC) $(LFLAGS) -o gribslim $(OBJS2) $(LIBS)

gribpick: $(OBJS3)
	$(CC) $(LFLAGS) -o gribpick $(OBJS3) $(LIBS)

testemap: emaprep.o plot.o
	$(CC) $(LFLAGS) -o testemap emaprep.o plot.o $(LIBS)

testv: testv.c visual.o
	$(CC) $(LFLAGS) -o testv testv.c visual.o $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $<

mymalloc.o: mymalloc.h
gribscan.o: gribscan.h mymalloc.h
mainlogic.o: gribscan.h visual.h mymalloc.h
visual.o: visual.h mymalloc.h
filter.o: gribscan.h

clean:
	rm -f $(OBJS) grib2png *.png

tags: mainlogic.c gribscan.c gribscan.h visual.c visual.h filter.c
	ctags mainlogic.c gribscan.c gribscan.h visual.c visual.h filter.c

OBJS=gribscan.o mainlogic.o trapbin.o trapsfc.o visual.o mymalloc.o
OBJS2=gribscan.o mainslim.o mymalloc.o filter.o
OBJS3=gribscan.o mainpick.o mymalloc.o filter.o
OBJS4=emagram.o emaprep.o plot.o
LIBS= -lm -lpng
# OPTS= -g -pg を想定
LFLAGS= $(OPTS) -fopenmp
CFLAGS= $(OPTS) -fopenmp -O2 --pedantic -std=gnu99 -Wall 
CC=cc

.SUFFIXES:
.SUFFIXES: .c .o

all: grib2png gribslim gribpick emagram

grib2png: $(OBJS)
	$(CC) $(LFLAGS) -o grib2png $(OBJS) $(LIBS)

gribslim: $(OBJS2)
	$(CC) $(LFLAGS) -o gribslim $(OBJS2) $(LIBS)

gribpick: $(OBJS3)
	$(CC) $(LFLAGS) -o gribpick $(OBJS3) $(LIBS)

emagram: $(OBJS4)
	$(CC) $(LFLAGS) -o emagram $(OBJS4) $(LIBS)

testv: testv.c visual.o
	$(CC) $(LFLAGS) -o testv testv.c visual.o $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $<

mymalloc.o: mymalloc.h
gribscan.o: gribscan.h mymalloc.h
mainlogic.o: grib2png.h gribscan.h visual.h mymalloc.h
trapbin.o: grib2png.h gribscan.h
trapsfc.o: grib2png.h gribscan.h
visual.o: visual.h mymalloc.h
filter.o: gribscan.h
plot.o: plot.h
emaprep.o: emagram.h plot.h
emagram.o: emagram.h

clean:
	rm -f $(OBJS) grib2png *.png

tags: mainlogic.c gribscan.c gribscan.h visual.c visual.h filter.c
	ctags *.c *.h

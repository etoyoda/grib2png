OBJS=gribscan.o mainlogic.o visual.o
LIBS= -lm -lpng
LFLAGS= -g
CFLAGS= -g --pedantic -std=gnu99 -Wall 
CC=cc

.SUFFIXES:
.SUFFIXES: .c .o

grib2png: $(OBJS)
	$(CC) $(LFLAGS) -o grib2png $(OBJS) $(LIBS)

testv: testv.c visual.o
	$(CC) $(LFLAGS) -o testv testv.c visual.o $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $<

gribscan.o: gribscan.h
mainlogic.o: gribscan.h visual.h
visual.o: visual.h

clean:
	rm -f $(OBJS) grib2png *.png

tags: mainlogic.c gribscan.c gribscan.h visual.c visual.h
	ctags mainlogic.c gribscan.c gribscan.h visual.c visual.h

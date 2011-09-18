LIBS=`pkg-config --libs xmms2-client`
GLIB=`pkg-config --libs xmms2-client-glib` `pkg-config --libs glib-2.0`

all: tut1 tut2 tut3 tut4 tut5 tut6 tut7 tut8 tut9 tut10 tut11 tut12

%.o: %.c
	gcc -g `pkg-config --cflags xmms2-client glib-2.0` -Wall -c $(CFLAGS) -o $@ $<

tut%: tut%.o
	gcc -o $@ $< $(LIBS)

tut6: tut6.o
	gcc -o $@ $< $(LIBS) $(GLIB)

tut7: tut7.o
	gcc -o $@ $< $(LIBS) $(GLIB)

tut8: tut8.o
	gcc -o $@ $< $(LIBS) $(GLIB)

tut9: tut9.o
	gcc -o $@ $< $(LIBS) $(GLIB)

tut10: tut10.o
	gcc -o $@ $< $(LIBS) $(GLIB)

tut11: tut11.o
	gcc -g  $(CFLAGS) -o $@ $< $(LIBS) $(GLIB)

tut12: tut12.o
	gcc -g  $(CFLAGS) -o $@ $< $(LIBS) $(GLIB)

clean:
	rm -f tut? tut10 *.o


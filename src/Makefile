include Makefile.inc

LOCALFLAGS=-g -Wall -Wno-unused

LIBS=$(BASELIBS)
LIBPATHS=$(BASELIBPATHS)
INCPATHS=$(BASEINCPATHS)
FLAGS=$(BASEFLAGS) $(LOCALFLAGS)

OBJECTS=screen.o log.o network.o parser.o ctcp.o dcc.o ncolor.o main.o cmenu.o config.o forms.o option.o


all: autodefs.h $(PROGRAMNAME)

$(PROGRAMNAME): $(OBJECTS)
	$(CC) $(FLAGS) -o $(PROGRAMNAME) $(OBJECTS) $(LIBPATHS) $(LIBS)

log.o: log.c log.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

config.o: config.c config.h defines.h common.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

main.o: main.c main.h common.h defines.h 
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

network.o: network.c network.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

screen.o: screen.c screen.h common.h defines.h 
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

server.o: server.c server.h common.h defines.h 
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

channel.o: channel.c channel.h common.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

ctcp.o: ctcp.c ctcp.h common.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

dcc.o: dcc.c dcc.h common.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

parser.o: parser.c parser.h common.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

ncolor.o: ncolor.c ncolor.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

cmenu.o: cmenu.c cmenu.h defines.h common.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

forms.o: forms.c forms.h defines.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

option.o: option.c option.h defines.h common.h
	$(CC) $(FLAGS) $(INCPATHS) -c -o $@ $<

html: html.c 
	$(CC) $(FLAGS) $(INCPATHS) -o $@ $<

autodefs.h:
	@echo "*********************************************************"
	@echo "*** Please run configure from the top most directory. ***"
	@echo "*********************************************************"

clean:
	rm -f $(PROGRAMNAME) *.o plog.log core*


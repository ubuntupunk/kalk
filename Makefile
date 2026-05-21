PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1

CFLAGS = -std=c99 -pedantic
LDFLAGS = -lcurses -lm

OBJS = cell.o formula.o csv.o editor.o

all: kalk test

cell.o: cell.c cell.h
	$(CC) $(CFLAGS) -c cell.c

formula.o: formula.c cell.h
	$(CC) $(CFLAGS) -c formula.c

csv.o: csv.c cell.h
	$(CC) $(CFLAGS) -c csv.c

editor.o: editor.c cell.h
	$(CC) $(CFLAGS) -c editor.c

kalk.o: kalk.c cell.h
	$(CC) $(CFLAGS) -c kalk.c

test.o: test.c cell.h
	$(CC) $(CFLAGS) -c test.c

kalk: kalk.o $(OBJS)
	$(CC) -o $@ kalk.o $(OBJS) $(LDFLAGS)

test: test.o formula.o cell.o csv.o
	$(CC) -o testkalk test.o formula.o cell.o csv.o -lm
	./testkalk

install: kalk
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 kalk $(DESTDIR)$(BINDIR)/kalk
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 kalk.1 $(DESTDIR)$(MANDIR)/kalk.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/kalk
	rm -f $(DESTDIR)$(MANDIR)/kalk.1

clean:
	rm -f kalk testkalk *.o

.PHONY: all kalk test install uninstall clean

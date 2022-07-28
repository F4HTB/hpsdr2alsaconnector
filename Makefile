CC = c++ 

EXECUTABLE = hpsdr2alsa

CFLAGS += -Wall -Wextra -march=native
LDLIBS += -lm -lasound -pthread

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin
datarootdir=$(prefix)/share
mandir=$(datarootdir)/man

.PHONY: all clean install installdirs

all: $(EXECUTABLE)

$(EXECUTABLE): $(EXECUTABLE).c $(EXECUTABLE).h

clean:
	rm -f $(EXECUTABLE)

install: $(EXECUTABLE) installdirs
	$(INSTALL_PROGRAM) $(EXECUTABLE) $(DESTDIR)$(bindir)/$(EXECUTABLE)


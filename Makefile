TARGET = ate.so
BUILTIN = $(basename $(TARGET))
ENABLER = $(addprefix enable_,$(BUILTIN))

PREFIX ?= /usr/local

# Change if source files not in base directory:
SRC = .

CFLAGS = -Wall -Werror -std=c99 -pedantic -ggdb
LFLAGS =
LDFLAGS =

# Uncomment the following if target is a Shared library
CFLAGS += -fPIC
LFLAGS += --shared

CFLAGS += -I/usr/include/bash -I/usr/include/bash/include

# Build module list (info make -> "Functions" -> "File Name Functions")
MODULES = $(addsuffix .o,$(basename $(wildcard $(SRC)/*.c)))

# Libraries need header files.  Set the following accordingly:
HEADERS =

# Declare non-filename targets
.PHONY: all install uninstall clean help

all: $(TARGET)

$(TARGET) : $(MODULES)
	$(CC) $(LFLAGS) -o $@ $(MODULES) $(LDFLAGS)

$(ENABLER):
	@echo "#!/usr/bin/env bash"                         > $(ENABLER)
	@echo "echo -f $(PREFIX)/lib/$(TARGET) $(BUILTIN)" >> $(ENABLER)

%o : %c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(ENABLER)
	install -D --mode=775 $(ENABLER) $(PREFIX)/bin
	install -D --mode=775 $(TARGET) $(PREFIX)/lib
	mkdir --mode=755 -p $(PREFIX)/share/man/man7
	soelim ate.1 | gzip -c - > $(PREFIX)/share/man/man7/ate.1.gz
	soelim ate.7 | gzip -c - > $(PREFIX)/share/man/man7/ate.7.gz
	soelim ate-examples.7 | gzip -c - > $(PREFIX)/share/man/man7/ate-examples.7.gz

uninstall:
	rm -f $(PREFIX)/bin/$(ENABLER)
	rm -f $(PREFIX)/lib/$(TARGET)
	rm -f $(PREFIX)/share/man/man1/ate.1.gz
	rm -f $(PREFIX)/share/man/man7/ate.7.gz
	rm -f $(PREFIX)/share/man/man7/ate-examples.7.gz

clean:
	rm -f $(TARGET)
	rm -f $(ENABLER)
	rm -f $(MODULES)
	rm -f examples/uscounties.csv
	rm -f examples/simplemaps*

help:
	@echo "Makefile options:"
	@echo
	@echo "  install    to install project"
	@echo "  uninstall  to uninstall project"
	@echo "  clean      to remove generated files"
	@echo "  help       this display"

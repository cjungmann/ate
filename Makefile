TARGET_ROOT = ate
TARGET = $(TARGET_ROOT).so
BUILTIN = $(TARGET_ROOT)
ENABLER = $(addprefix enable_,$(TARGET_ROOT))
SOURCER = $(addprefix $(TARGET_ROOT),_sources)

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

HEADERS = $(wildcard $(SRC)/*.h)

# Declare non-filename targets
.PHONY: all install uninstall clean help

all: $(TARGET) ate_sources.d/ate_mimes

$(TARGET) : $(MODULES)
	$(CC) $(LFLAGS) -o $@ $(MODULES) $(LDFLAGS)

ate_sources.d/ate_mimes: ate_sources.d/ate_mimes.def ate_sources.d/d.ate_mimes/*
	./desource $< > $@

$(ENABLER):
	@echo "#!/usr/bin/env bash"                         > $(ENABLER)
	@echo "echo -f $(PREFIX)/lib/$(TARGET) $(BUILTIN)" >> $(ENABLER)

*.c: *.h
	@echo "Forcing full recompile after any header file change"
	touch *.c

%o: %c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(ENABLER)
	install -D --mode=775 $(ENABLER) $(PREFIX)/bin
	install -D --mode=775 $(TARGET) $(PREFIX)/lib
	mkdir --mode=755 -p $(PREFIX)/share/man/man1
	mkdir --mode=755 -p $(PREFIX)/share/man/man7
	soelim ate.1 | gzip -c - > $(PREFIX)/share/man/man1/ate.1.gz
	soelim ate.7 | gzip -c - > $(PREFIX)/share/man/man7/ate.7.gz
	soelim ate-examples.7 | gzip -c - > $(PREFIX)/share/man/man7/ate-examples.7.gz
	# install SOURCER and sources
	rm -f $(PREFIX)/bin/$(SOURCER)
	sed -e s^#PREFIX#^$(PREFIX)^ -e s^#BUILTIN#^$(BUILTIN)^ $(SOURCER) > $(PREFIX)/bin/$(SOURCER)
	chmod a+x $(PREFIX)/bin/$(SOURCER)
	install -D $(BUILTIN)_sources.d/ate_* -t$(PREFIX)/lib/$(BUILTIN)_sources

uninstall:
	rm -f $(PREFIX)/bin/$(ENABLER)
	rm -f $(PREFIX)/lib/$(TARGET)
	rm -f $(PREFIX)/share/man/man1/ate.1.gz
	rm -f $(PREFIX)/share/man/man7/ate.7.gz
	rm -f $(PREFIX)/share/man/man7/ate-examples.7.gz
# uninstall SOURCER stuff:
	rm -rf $(PREFIX)/lib/$(BUILTIN)_sources
	rm -f $(PREFIX)/bin/$(SOURCER)

clean:
	rm -f $(TARGET)
	rm -f $(ENABLER)
	rm -f $(MODULES)

help:
	@echo "Makefile options:"
	@echo
	@echo "  install    to install project"
	@echo "  uninstall  to uninstall project"
	@echo "  clean      to remove generated files"
	@echo "  help       this display"

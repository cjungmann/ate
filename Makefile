TARGET_ROOT = ate
TARGET = $(TARGET_ROOT)
BUILTIN = $(TARGET_ROOT)
SOURCER = $(addprefix $(TARGET_ROOT),_sources)

PREFIX ?= /usr/local

# Change if source files not in base directory:
SRC = .

# These will be set by ./configure
LIB_FLAGS =
LIB_MODULES =

CFLAGS = -Wall -Werror -std=c99 -pedantic -ggdb
LFLAGS =
LDFLAGS = -lm $(LIB_FLAGS)
INCFLAGS =

# Uncomment the following if target is a Shared library
CFLAGS += -fPIC
LFLAGS += --shared

CFLAGS += -I/usr/include/bash -I/usr/include/bash/include $(INCFLAGS)

# Build module list (info make -> "Functions" -> "File Name Functions")
MODULES = $(addsuffix .o,$(basename $(wildcard $(SRC)/*.c)))

HEADERS = $(wildcard $(SRC)/*.h)

# Declare non-filename targets
.PHONY: all install uninstall clean help

all: $(TARGET) ate_sources.d/ate_mimes

$(TARGET) : $(MODULES)
ifndef LIB_FLAGS
ifndef LIB_MODULES
	$(error "Run 'configure' to setup libraries")
endif
endif
	$(CC) $(LFLAGS) -o $@ $(MODULES) $(LIB_MODULES) $(LDFLAGS)

ate_sources.d/ate_mimes: ate_sources.d/def.ate_mimes ate_sources.d/d.ate_mimes/*
	./desource $< > $@

*.c: *.h
	@echo "Forcing full recompile after any header file change"
	touch *.c

%o: %c
	$(CC) $(CFLAGS) -c -o $@ $<

install:
	install -D --mode=775 $(TARGET) -t $(PREFIX)/lib/bash
	mkdir --mode=755 -p $(PREFIX)/share/man/man1
	mkdir --mode=755 -p $(PREFIX)/share/man/man7
	soelim $(TARGET_ROOT).1 | gzip -c - > $(PREFIX)/share/man/man1/$(TARGET_ROOT).1.gz
	soelim $(TARGET_ROOT).7 | gzip -c - > $(PREFIX)/share/man/man7/$(TARGET_ROOT).7.gz
	soelim ate-examples.7 | gzip -c - > $(PREFIX)/share/man/man7/ate-examples.7.gz
	# install SOURCER and sources
	rm -f $(PREFIX)/bin/$(SOURCER)_impl
	rm -f $(PREFIX)/bin/$(SOURCER)
	sed -e s^#PREFIX#^$(PREFIX)^ -e s^#BUILTIN#^$(BUILTIN)^ $(SOURCER) > $(PREFIX)/bin/$(SOURCER)_impl
	chmod a+x $(PREFIX)/bin/$(SOURCER)_impl
	cp -s $(PREFIX)/bin/$(SOURCER)_impl $(PREFIX)/bin/$(SOURCER)
	install -D $(BUILTIN)_sources.d/$(BUILTIN)_* -t$(PREFIX)/lib/$(BUILTIN)_sources

uninstall:
	rm -f $(PREFIX)/lib/$(TARGET)
	rm -f $(PREFIX)/share/man/man1/$(TARGET_ROOT).1.gz
	rm -f $(PREFIX)/share/man/man7/$(TARGET_ROOT).7.gz
	rm -f $(PREFIX)/share/man/man7/ate-examples.7.gz
# uninstall SOURCER stuff:
	rm -rf $(PREFIX)/lib/$(BUILTIN)_sources
	rm -f $(PREFIX)/bin/$(SOURCER)
	rm -f $(PREFIX)/bin/$(SOURCER)_impl

clean:
	rm -f $(TARGET)
	rm -f $(MODULES)

help:
	@echo "Makefile options:"
	@echo
	@echo "  install    to install project"
	@echo "  uninstall  to uninstall project"
	@echo "  clean      to remove generated files"
	@echo "  help       this display"

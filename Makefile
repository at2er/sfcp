CC = gcc
AR = ar
PKG_CONFIG = pkg-config
PREFIX = /usr/local

PORTAL = sfcp.portal
TARGET = sfcp
WRAP = wrap-sfcp-fm

SRC = sfcp.c
OBJ = $(SRC:.c=.o)

PKGS = dbus-1
LIBS = `$(PKG_CONFIG) --libs $(PKGS)`
INCS = `$(PKG_CONFIG) --cflags $(PKGS)`
CFLAGS  = -Wall -std=c99 $(INCS)
LDFLAGS = $(LIBS)

.PHONY: all clean install uninstall
all: $(TARGET)

config.h: config.def.h
	cp -f $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): config.h $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install: all
	mkdir -p $(PREFIX)/share/xdg-desktop-portal/portals
	cp -f $(PORTAL) $(PREFIX)/share/xdg-desktop-portal/portals/$(PORTAL)
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)
	cp -f $(WRAP) $(PREFIX)/bin/$(WRAP)

uninstall:
	rm -f $(PREFIX)/share/xdg-desktop-portal/portals/$(PORTAL) \
		$(PREFIX)/bin/$(TARGET) \
		$(PREFIX)/bin/$(WRAP)

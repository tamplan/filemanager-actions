prefix=/usr
libdir=$(prefix)/lib
bindir=${prefix}/bin
DESTDIR=
GNOME_MENU_DIR=$(prefix)/share/applications
DEFAULT_CONFIG_PATH=$(prefix)/share/nautilus-actions
DEFAULT_PER_USER_PATH=.nautilus-actions
DEFAULT_NACT_DATA_PATH=$(DEFAULT_CONFIG_PATH)/nact
EXTDIR=$(libdir)/nautilus/extensions-1.0
EXT_NAME=nautilus-action.so
SOURCES=nautilus-actions.c nautilus-actions-config.c nautilus-actions-module.c nautilus-actions-test.c nautilus-actions-utils.c
HEADERS=nautilus-actions-config.h nautilus-actions.h nautilus-actions-test.h nautilus-actions-utils.h
VERSION=0.4

all: $(EXT_NAME) nact/nautilus-actions-config.py nact/nact nact/nautilus-actions-config.glade nact/nact.desktop

$(EXT_NAME): $(SOURCES) $(HEADERS)
	gcc -o $(EXT_NAME) -shared -fPIC -DDEFAULT_CONFIG_PATH="\"$(DEFAULT_CONFIG_PATH)\"" -DDEFAULT_PER_USER_PATH="\"$(DEFAULT_PER_USER_PATH)\"" -I. `pkg-config --libs --cflags libnautilus-extension libxml-2.0 gobject-2.0` $(SOURCES)

nact/nautilus-actions-config.py: nact/nautilus-actions-config.py.template
	sed -e 's#%%%DEFAULT_CONFIG_PATH%%%#$(DEFAULT_CONFIG_PATH)#' -e 's#%%%DATA_DIR%%%#$(DEFAULT_NACT_DATA_PATH)#' -e 's#%%%VERSION%%%#$(VERSION)#' -e 's#%%%DEFAULT_PER_USER_PATH%%%#$(DEFAULT_PER_USER_PATH)#' nact/nautilus-actions-config.py.template > nact/nautilus-actions-config.py

nact/nact: nact/nact.template
	sed 's#%%%DATA_DIR%%%#$(DEFAULT_NACT_DATA_PATH)#' nact/nact.template > nact/nact

nact/nautilus-actions-config.glade: nact/nautilus-actions-config_dev.glade
	sed -e 's#nautilus-launch-icon.png#$(DEFAULT_NACT_DATA_PATH)/nautilus-launch-icon.png#' nact/nautilus-actions-config_dev.glade > nact/nautilus-actions-config.glade

nact/nact.desktop: nact/nact.desktop.template
	sed -e 's#%%%DATA_DIR%%%#$(DEFAULT_NACT_DATA_PATH)#' -e 's#%%%VERSION%%%#$(VERSION)#' nact/nact.desktop.template > nact/nact.desktop

install:
	/usr/bin/install -d -m 755 $(DESTDIR)$(DEFAULT_NACT_DATA_PATH)
	/usr/bin/install -d -m 755 $(DESTDIR)$(EXTDIR)
	/usr/bin/install -d -m 755 $(DESTDIR)$(bindir)
	/usr/bin/install -d -m 755 $(DESTDIR)$(GNOME_MENU_DIR)
	/usr/bin/install -m 755 $(EXT_NAME) $(DESTDIR)$(EXTDIR)
	/usr/bin/install -m 755 nact/nact $(DESTDIR)$(bindir)
	/usr/bin/install -m 644 nact/nautilus-actions-config.glade nact/nautilus-launch-icon.png $(DESTDIR)$(DEFAULT_NACT_DATA_PATH)
	/usr/bin/install -m 755 nact/nautilus-actions-config.py $(DESTDIR)$(DEFAULT_NACT_DATA_PATH)
	/usr/bin/install -m 644 config/config_newaction.xml $(DESTDIR)$(DEFAULT_CONFIG_PATH)
	/usr/bin/install -m 644 nact/nact.desktop $(DESTDIR)$(GNOME_MENU_DIR)

clean:
	rm -f $(EXT_NAME) nact/nautilus-actions-config.py nact/nact nact/nact.desktop nact/nautilus-actions-config.glade  *~ *.o nact/*.bak nact/*~

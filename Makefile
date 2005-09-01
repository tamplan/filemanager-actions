prefix=/usr
DEFAULT_CONFIG_PATH=$(prefix)/share/nautilus-actions
DEFAULT_NACT_DATA_PATH=$(DEFAULT_CONFIG_PATH)/nact
EXTDIR=$(prefix)/lib/nautilus/extensions-1.0
EXT_NAME=nautilus-action.so
SOURCES=nautilus-actions.c nautilus-actions-config.c nautilus-actions-module.c nautilus-actions-test.c nautilus-actions-utils.c
HEADERS=nautilus-actions-config.h nautilus-actions.h nautilus-actions-test.h nautilus-actions-utils.h
VERSION=0.3
bindir=${prefix}/bin

all: $(EXT_NAME) nact/nautilus-actions-config.py nact/nact nact/nautilus-actions-config.glade

$(EXT_NAME): $(SOURCES) $(HEADERS)
	gcc -o $(EXT_NAME) -shared -DDEFAULT_CONFIG_PATH="\"$(DEFAULT_CONFIG_PATH)\"" -I. `pkg-config --libs --cflags libnautilus-extension libxml-2.0 gobject-2.0` $(SOURCES)

nact/nautilus-actions-config.py: nact/nautilus-actions-config.py.template
	sed -e 's#%%%DEFAULT_CONFIG_PATH%%%#$(DEFAULT_CONFIG_PATH)#' -e 's#%%%DATA_DIR%%%#$(DEFAULT_NACT_DATA_PATH)#' -e 's#%%%VERSION%%%#$(VERSION)#' nact/nautilus-actions-config.py.template > nact/nautilus-actions-config.py

nact/nact: nact/nact.template
	sed 's#%%%DATA_DIR%%%#$(DEFAULT_NACT_DATA_PATH)#' nact/nact.template > nact/nact

nact/nautilus-actions-config.glade: nact/nautilus-actions-config_dev.glade
	sed -e 's#nautilus-launch-icon.png#$(DEFAULT_NACT_DATA_PATH)/nautilus-launch-icon.png#' nact/nautilus-actions-config_dev.glade > nact/nautilus-actions-config.glade

install:
	/usr/bin/install -d -m 755 $(DEFAULT_NACT_DATA_PATH)
	/usr/bin/install -d -m 755 $(EXTDIR)
	/usr/bin/install -d -m 755 $(bindir)
	/usr/bin/install -m 755 $(EXT_NAME) $(EXTDIR)
	/usr/bin/install -m 755 nact/nact $(bindir)
	/usr/bin/install -m 644 nact/nautilus-actions-config.glade nact/nautilus-launch-icon.png $(DEFAULT_NACT_DATA_PATH)
	/usr/bin/install -m 755 nact/nautilus-actions-config.py $(DEFAULT_NACT_DATA_PATH)

clean:
	rm -f $(EXT_NAME) nact/nautilus-actions-config.py nact/nact nact/nautilus-actions-config.glade  *~ *.o nact/*.bak nact/*~

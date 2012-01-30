#!/bin/sh
#
# let's check the NATracker service

NA_SERVICE=$(grep '#define NAUTILUS_ACTIONS_DBUS_SERVICE' src/api/na-dbus.h | awk '{ print $3 }' | sed 's?"??g')
NA_TRACKER_PATH=$(grep '#define NAUTILUS_ACTIONS_DBUS_TRACKER_PATH' src/api/na-dbus.h | awk '{ print $3 }' | sed 's?"??g')/0

echo "D-Bus service      =  ${NA_SERVICE}"
echo "D-Bus tracker path = ${NA_TRACKER_PATH}"
echo ""
dbus-send --session --type=method_call --print-reply --dest=${NA_SERVICE} ${NA_TRACKER_PATH} org.freedesktop.DBus.Introspectable.Introspect

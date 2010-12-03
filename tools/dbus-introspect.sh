#!/bin/sh
#
# let's check the NATracker service

NA_SERVICE=org.nautilus-actions.DBus
NA_PATH_TRACKER=/org/nautilus_actions/DBus/Tracker

echo ""
echo "D-Bus service      =  ${NA_SERVICE}"
echo "D-Bus tracker path = ${NA_PATH_TRACKER}"
dbus-send --session --type=method_call --print-reply --dest=${NA_SERVICE} ${NA_PATH_TRACKER} org.freedesktop.DBus.Introspectable.Introspect

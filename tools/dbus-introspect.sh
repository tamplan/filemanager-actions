#!/bin/sh
#
# let's check the FMATracker service
#
FMA_SERVICE=$(grep '#define FILEMANAGER_ACTIONS_DBUS_SERVICE' src/api/fma-dbus.h | awk '{ print $3 }' | sed 's?"??g')
FMA_TRACKER_PATH=$(grep '#define FILEMANAGER_ACTIONS_DBUS_TRACKER_PATH' src/api/fma-dbus.h | awk '{ print $3 }' | sed 's?"??g')/0
echo "
D-Bus service     : ${FMA_SERVICE}
D-Bus tracker path: ${FMA_TRACKER_PATH}
"
dbus-send --session --type=method_call --print-reply --dest=${FMA_SERVICE} ${FMA_TRACKER_PATH} org.freedesktop.DBus.Introspectable.Introspect

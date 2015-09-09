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
#
# Expected display:
#
# D-Bus service     : org.filemanager-actions.DBus
# D-Bus tracker path: /org/filemanager_actions/DBus/Tracker/0
# 
# method return sender=:1.107 -> dest=:1.115 reply_serial=2
#    string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
#                       "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
# <!-- GDBus 2.44.1 -->
# <node>
#   <interface name="org.freedesktop.DBus.Properties">
#     <method name="Get">
#       <arg type="s" name="interface_name" direction="in"/>
#       <arg type="s" name="property_name" direction="in"/>
#       <arg type="v" name="value" direction="out"/>
#     </method>
#     <method name="GetAll">
#       <arg type="s" name="interface_name" direction="in"/>
#       <arg type="a{sv}" name="properties" direction="out"/>
#     </method>
#     <method name="Set">
#       <arg type="s" name="interface_name" direction="in"/>
#       <arg type="s" name="property_name" direction="in"/>
#       <arg type="v" name="value" direction="in"/>
#     </method>
#     <signal name="PropertiesChanged">
#       <arg type="s" name="interface_name"/>
#       <arg type="a{sv}" name="changed_properties"/>
#       <arg type="as" name="invalidated_properties"/>
#     </signal>
#   </interface>
#   <interface name="org.freedesktop.DBus.Introspectable">
#     <method name="Introspect">
#       <arg type="s" name="xml_data" direction="out"/>
#     </method>
#   </interface>
#   <interface name="org.freedesktop.DBus.Peer">
#     <method name="Ping"/>
#     <method name="GetMachineId">
#       <arg type="s" name="machine_uuid" direction="out"/>
#     </method>
#   </interface>
#   <interface name="org.filemanager_actions.DBus.Tracker.Properties1">
#     <method name="GetSelectedPaths">
#       <arg type="as" name="paths" direction="out"/>
#     </method>
#   </interface>
# </node>

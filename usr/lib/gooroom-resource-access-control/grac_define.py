#! /usr/bin/env python3

#-----------------------------------------------------------------------

#full path of config
CONFIG_FULLPATH='/etc/gooroom/grac.d/Grac.conf'

#DBUS
DBUS_NAME='dbus_name'
DBUS_OBJ='dbus_obj'
DBUS_IFACE='dbus_iface'

#grac return code
GRAC_OK='200'
GRAC_NOK='111'

#rules map
RULES_MAP_MODE = 'MODE'
RULES_MAP_FILTER = 'FILTER'
RULES_MAP_TYPE_ENV = 'ENV'
RULES_MAP_TYPE_ATTR = 'ATTR'
RULES_MAP_TYPE_MODULE = 'MODULE'
RULES_MAP_TYPE_PROP = 'PROP'
RULES_MAP_TYPE_ATTRS = 'ATTRS'

#json rules
JSON_RULE_ALLOW = 'allow'
JSON_RULE_DISALLOW = 'disallow'
JSON_RULE_READONLY = 'read_only'
JSON_RULE_STATE = 'state'
JSON_RULE_USB_MEMORY = 'usb_memory'
JSON_RULE_PRINTER = 'printer'
JSON_RULE_SOUND = 'sound'
JSON_RULE_MICROPHONE = 'microphone'
JSON_RULE_MOUSE = 'mouse'
JSON_RULE_KEYBOARD = 'keyboard'
JSON_RULE_CD_DVD = 'cd_dvd'
JSON_RULE_BLUETOOTH = 'bluetooth'
JSON_RULE_BLUETOOTH_MAC = 'mac_address'
JSON_RULE_WIRELESS = 'wireless'
JSON_RULE_CAMERA = 'camera'
JSON_RULE_NETWORK = 'network'
JSON_RULE_NETWORK_STATE = 'state'
JSON_RULE_NETWORK_RULES = 'rules'
JSON_RULE_NETWORK_IPADDRESS = 'ipaddress'
JSON_RULE_NETWORK_PORTS = 'ports'
JSON_RULE_NETWORK_SRC_PORT = 'src_port'
JSON_RULE_NETWORK_DST_PORT = 'dst_port'
JSON_RULE_NETWORK_PROTOCOL = 'protocol'
JSON_RULE_NETWORK_DIRECTION = 'direction'
JSON_RULE_NETWORK_INBOUND = 'inbound'
JSON_RULE_NETWORK_OUTBOUND = 'outbound'
JSON_RULE_NETWORK_ALLBOUND = 'all'

#delimeter
GRAC_PRINTER_DELIM = ' +GRAC-PRINTER-DELIM+ ' #watch out spaces

#systemd dbus info
SD_BUS_NAME = 'org.freedesktop.systemd1'
SD_BUS_OBJ = '/org/freedesktop/systemd1'
SD_BUS_IFACE = 'org.freedesktop.systemd1.Manager'

#reverse lookup limit
REVERSE_LOOKUP_LIMIT = 4

#meta files
META_ROOT = '/etc/gooroom/grac.d'
META_FILE_PCI_RESCAN = META_ROOT+'/pci_rescan'
META_FILE_PRINTER_BACKUP_PATH = META_ROOT+'/printers'
META_FILE_PRINTER_LIST = 'printer-list'
META_SERVER_CERT_PATH = '/etc/gooroom/agent/server_certificate.crt'
META_SIGNATURE_PATH = '/var/tmp/gooroom-agent-service'

#media control type
MC_TYPE_AUTH = 'authorized'
MC_TYPE_REMOVE = 'remove'
MC_TYPE_NA = 'na'
MC_TYPE_BCONFIG = 'bConfigurationValue'
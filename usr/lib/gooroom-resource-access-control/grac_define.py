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
JSON_RULE_USB_MEMORY_WHITELIST = 'usb_serialno'
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
JSON_RULE_CLIPBOARD = 'clipboard'
JSON_RULE_SCREEN_CAPTURE = 'screen_capture'
JSON_RULE_NETWORK = 'network'
JSON_RULE_NETWORK_STATE = 'state'
JSON_RULE_NETWORK_RULES = 'rules'
JSON_RULE_NETWORK_IPADDRESS = 'ipaddress'
JSON_RULE_NETWORK_PORTS = 'ports'
JSON_RULE_NETWORK_SRC_PORT = 'src_port'
JSON_RULE_NETWORK_DST_PORT = 'dst_port'
JSON_RULE_NETWORK_SRC_PORTS = 'src_ports'
JSON_RULE_NETWORK_DST_PORTS = 'dst_ports'
JSON_RULE_NETWORK_PROTOCOL = 'protocol'
JSON_RULE_NETWORK_DIRECTION = 'direction'
JSON_RULE_NETWORK_INBOUND = 'input'
JSON_RULE_NETWORK_OUTBOUND = 'output'
JSON_RULE_NETWORK_ALLBOUND = 'all'
JSON_RULE_NETWORK_ACCEPT = 'accept'
JSON_RULE_NETWORK_VERSION = 'version'

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
MC_TYPE_NOSYNC = 'nosync'
MC_TYPE_ALLOWSYNC = 'allowsync'

#journal log level
JLEVEL_DEFAULT_NOTI = 2
JLEVEL_DEFAULT_SHOW = 5

#grmcode
GRMCODE_SIGNATURE_SUCCESS = '040006'
GRMCODE_SIGNATURE_FAIL = '040007'

GRMCODE_USB_MEMORY_DISALLOW = '040008'
GRMCODE_USB_MEMORY_READONLY = '040009'
GRMCODE_PRINTER_DISALLOW = '040010'
GRMCODE_CD_DVD_DISALLOW = '040011'
GRMCODE_CAMERA_DISALLOW = '040012'
GRMCODE_SOUND_DISALLOW = '040013'
GRMCODE_MICROPHONE_DISALLOW = '040014'
GRMCODE_WIRELESS_DISALLOW = '040015'
GRMCODE_BLUETOOTH_DISALLOW = '040016'
GRMCODE_KEYBOARD_DISALLOW = '040017'
GRMCODE_MOUSE_DISALLOW = '040018'

#red-alert flag
RED_ALERT_JOURNALONLY = 'journalonly'
RED_ALERT_ALERTONLY = 'alertonly'
RED_ALERT_ALL = 'all'

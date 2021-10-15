#!/usr/bin/env python3

#-------------------------------------------------------------------------------

#path
DEFAULT_RULES_PATH = '/etc/gooroom/grac.d/default.rules'
GLADE_PATH = '/usr/lib/grac-editor/grac_editor.glade'

#rule items
JSON_RULE_ALLOW = 'allow'
JSON_RULE_DISALLOW = 'disallow'
JSON_RULE_READONLY = 'readonly'

JSON_RULE_STATE = 'state'
JSON_RULE_MAC_ADDRESS = 'mac_address'
JSON_RULE_USB_SERIALNO = 'usb_serialno'
JSON_RULE_WHITELIST = 'whitelist'
JSON_RULE_USB_MEMORY = 'usb_memory'
JSON_RULE_BLUETOOTH = 'bluetooth'
JSON_RULE_NETWORK = 'network'
JSON_RULE_NETWORK_RULE = 'rules'
JSON_RULE_NETWORK_ADDRESS = 'ipaddress'
JSON_RULE_NETWORK_PORTS = 'ports'
JSON_RULE_NETWORK_DIRECTION = 'direction'
JSON_RULE_NETWORK_PROTO = 'protocol'
JSON_RULE_NETWORK_SRC_PORT = 'src_port'
JSON_RULE_NETWORK_DST_PORT = 'dst_port'
JSON_RULE_NETWORK_STATE = 'state'
JSON_RULE_NETWORK_VERSION = 'version'
JSON_RULE_NETWORK_SRC_PORTS = 'src_ports'
JSON_RULE_NETWORK_DST_PORTS = 'dst_ports'
JSON_RULE_USB_NETWORK = 'usb_network'
JSON_RULE_USB_NETWORK_WHITELIST = 'whitelist'
JSON_RULE_NETWORK_RULES_RAW = 'rules_raw'
JSON_RULE_PRINTER = 'printer'
JSON_RULE_SOUND = 'sound'
JSON_RULE_MICROPHONE = 'microphone'
JSON_RULE_MOUSE = 'mouse'
JSON_RULE_KEYBOARD = 'keyboard'
JSON_RULE_CD_DVD = 'cd_dvd'
JSON_RULE_BLUETOOTH = 'bluetooth'
JSON_RULE_CAMERA = 'camera'
JSON_RULE_CLIPBOARD = 'clipboard'
JSON_RULE_SCREEN_CAPTURE = 'screen_capture'
JSON_RULE_WIRELESS = 'wireless'
JSON_RULE_BLUETOOTH = 'bluetooth'
JSON_RULE_MOUSE = 'mouse'
JSON_RULE_KEYBOARD = 'keyboard'
JSON_RULE_USB_NETWORK ='usb_network'

#network
NETWORK_ACCEPT = 'accept'
NETWORK_DROP = 'drop'

#title for whitelist 
TITLE_WL_USB_MEMORY = 'usb memory'
TITLE_WL_BLUETOOTH = 'bluetooth'
TITLE_NETWORK_ADD = 'network add'
TITLE_NETWORK_EDIT = 'network edit'
TITLE_USB_NETWORK_WHITELIST_ADD = 'usb network whitelist add'
TITLE_USB_NETWORK_WHITELIST_EDIT = 'usb network whitelist edit'
#dbus
DBUS_NAME = 'kr.gooroom.GRACDEVD'
DBUS_OBJ = '/kr/gooroom/GRACDEVD'
DBUS_IFACE= 'kr.gooroom.GRACDEVD'

#validation
VALIDATE_OK = 'ok'

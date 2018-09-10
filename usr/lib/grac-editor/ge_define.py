#!/usr/bin/env python3

#-------------------------------------------------------------------------------

#path
DEFAULT_RULES_PATH = '/etc/gooroom/grac.d/default.rules'
GLADE_PATH = '/usr/lib/grac-editor/grac_editor.glade'

#rule items
JSON_RULE_ALLOW = 'allow'
JSON_RULE_DISALLOW = 'disallow'

JSON_RULE_STATE = 'state'
JSON_RULE_MAC = 'mac_address'
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

#network
NETWORK_ACCEPT = 'accept'
NETWORK_DROP = 'drop'

#title for whitelist 
TITLE_WL_USB_MEMORY = 'usb memory'
TITLE_WL_BLUETOOTH = 'bluetooth'
TITLE_NETWORK_ADD = 'network add'
TITLE_NETWORK_EDIT = 'network edit'

#dbus
DBUS_NAME = 'kr.gooroom.GRACDEVD'
DBUS_OBJ = '/kr/gooroom/GRACDEVD'
DBUS_IFACE= 'kr.gooroom.GRACDEVD'

#validation
VALIDATE_OK = 'ok'

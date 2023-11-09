#!/usr/bin/env python3

#-------------------------------------------------------------------------------
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

import simplejson as json
import traceback
import gettext
import shutil
import signal
import dbus
import sys
import os

from ge_define import *

gettext.install('grac-editor', '/usr/share/gooroom/locale')

#-------------------------------------------------------------------------------
class GracEditor:
    """
    GRAC EDITOR
    """

    def __init__(self):

        #SET DESKTOP ICON
        Gdk.set_program_class('grac-editor')

        #LOAD RULES
        self.media_rules = self.load_rules(DEFAULT_RULES_PATH)

        #BUILDER
        self.builder = Gtk.Builder()
        self.builder.add_from_file(GLADE_PATH)
        self.builder.connect_signals(self)

        #WINDOW MAIN
        self.window_main = self.builder.get_object('window_main')

        #SHOW
        self.window_main.show_all()

        #DRAW MEDIA
        self.draw_media(self.media_rules)

        #DRAW NETWORK
        self.draw_network(self.media_rules)

        #LANG
        self.lang()

    def logging(self, txt):
        """
        write on textview_log
        """

        buff = self.builder.get_object('textview_log').get_buffer()
        ei = buff.get_end_iter()
        buff.insert(ei, txt+'\n')
        ei = buff.get_end_iter()
        mark = buff.create_mark('end', ei, False)
        buff = self.builder.get_object('textview_log').scroll_to_mark(
                                                        mark, 
                                                        0.05,
                                                        True,
                                                        0.0,0.0)

    def draw_network(self, rules):
        """
        setting network page as values of rules file
        """

        network_rule = rules[JSON_RULE_NETWORK]

        #set policy
        policy = network_rule[JSON_RULE_STATE]
        if policy == JSON_RULE_ALLOW \
            or policy == NETWORK_ACCEPT:
            policy = NETWORK_ACCEPT
        else:
            policy = NETWORK_DROP

        policy_cb = self.builder.get_object('rdo_policy_{}'.format(policy))
        policy_cb.set_active(True)

        #set treeview
        treeview = self.builder.get_object('treeview_network')

        liststore_network = self.builder.get_object('liststore_network')
        liststore_network.clear()

        if not JSON_RULE_NETWORK_RULE in network_rule:
            return

        rules = network_rule[JSON_RULE_NETWORK_RULE]
        if not rules or len(rules) <= 0:
            return

        #sort by address
        cell_renderer = Gtk.CellRendererText()
        col = Gtk.TreeViewColumn('ADDRESS', cell_renderer)
        col.set_sort_column_id(2)

        if JSON_RULE_NETWORK_VERSION in network_rule:
            server_version = network_rule[JSON_RULE_NETWORK_VERSION]
        else:
            server_version = '1.0'

        for rule in rules:
            #SERVER VERSION 1.0
            if server_version.startswith('1.0'):
                address = rule[JSON_RULE_NETWORK_ADDRESS]
                state = rule[JSON_RULE_NETWORK_STATE]
                if state == JSON_RULE_ALLOW:
                    state = NETWORK_ACCEPT
                else:
                    state = NETWORK_DROP
                state = state.upper()

                direction = rule[JSON_RULE_NETWORK_DIRECTION].upper()

                if not JSON_RULE_NETWORK_PORTS in rule \
                    or len(rule[JSON_RULE_NETWORK_PORTS]) <= 0:
                    liststore_network.append([direction, '', address, '', '', state])
                    continue
                ports = rule[JSON_RULE_NETWORK_PORTS]

                for port in ports:
                    src_port = dst_port = ''
                    if JSON_RULE_NETWORK_SRC_PORT in port:
                        src_port = ','.join(port[JSON_RULE_NETWORK_SRC_PORT])
                    if JSON_RULE_NETWORK_DST_PORT in port:
                        dst_port = ','.join(port[JSON_RULE_NETWORK_DST_PORT])
                    proto = port[JSON_RULE_NETWORK_PROTO].upper()
                    liststore_network.append([direction, proto, address, src_port, dst_port, state])
            #SERVER NOT VERSION 1.0
            else:
                address = rule[JSON_RULE_NETWORK_ADDRESS]
                state = rule[JSON_RULE_NETWORK_STATE]
                if state == JSON_RULE_ALLOW \
                    or state == NETWORK_ACCEPT:
                    state = NETWORK_ACCEPT
                else:
                    state = NETWORK_DROP
                state = state.upper()

                direction = rule[JSON_RULE_NETWORK_DIRECTION].upper()
                proto = rule[JSON_RULE_NETWORK_PROTO].upper()
                src_ports = rule[JSON_RULE_NETWORK_SRC_PORTS]
                dst_ports = rule[JSON_RULE_NETWORK_DST_PORTS]
                liststore_network.append([direction, proto, address, src_ports, dst_ports, state])

    def draw_media(self, rules):
        """
        setting media page as the values of rules file
        """
    
        for k, v in rules.items():
            if isinstance(v, list):
                etc_v = None
                for tk, tv in v[0].items():
                    etc_v = tv
                    break
                if not etc_v:
                    continue
                vid = etc_v['items'][0]['vid']
                pid = etc_v['items'][0]['pid']
                if vid:
                    self.builder.get_object('ent_vendor').set_text(vid)
                if pid:
                    self.builder.get_object('ent_product').set_text(pid)

                obj_name = 'rdo+{}+{}'.format(k, etc_v['state'])
            else:
                obj_name = 'rdo+{}+{}'.format(k, v['state'])
            '''
            #microphone exception
            if obj_name == 'rdo+microphone+allow' \
                or obj_name == 'rdo+microphone+disallow':
                continue
            '''
            cb = self.builder.get_object(obj_name)
            if cb:
                cb.set_active(True)
            else:
                pass

        if self.builder.get_object('rdo+usb_memory+allow').get_active():
            self.builder.get_object('btn_usb_memory').set_sensitive(False)
        else:
            self.builder.get_object('btn_usb_memory').set_sensitive(True)

        if self.builder.get_object('rdo+bluetooth+allow').get_active():
            self.builder.get_object('btn_bluetooth').set_sensitive(False)
        else:
            self.builder.get_object('btn_bluetooth').set_sensitive(True)

        if self.builder.get_object('rdo+usb_network+allow').get_active():
            self.builder.get_object('btn_usb_network').set_sensitive(False)
        else:
            self.builder.get_object('btn_usb_network').set_sensitive(True)

    def load_rules(self, rule_path):
        """
        load rules file
        """
        
        with open(rule_path) as f:
            json_rules = json.loads(f.read())

        new_json_rules = {}

        for k, v in json_rules.items():
            state = None
            if isinstance(v, str):
                new_json_rules[k] = {}
                new_json_rules[k][JSON_RULE_STATE] = v
            elif isinstance(v, list):
                new_json_rules[k] = v
            else:
                new_json_rules[k] = {}
                for k2, v2 in v.items():
                    new_json_rules[k][k2] = v2

        return new_json_rules

    def on_btn_dialog_unw_input_ok_clicked(self, obj):
        """
        ok button of unw input dialog
        """

        dg = self.builder.get_object('dialog_unw_input')
        title = dg.get_title()

        name = 'usbbus'
        contents = self.builder.get_object('ent_unw_contents').get_text()

        if not name or not contents:
            md = Gtk.MessageDialog(
                None, 
                0, 
                Gtk.MessageType.WARNING, 
                Gtk.ButtonsType.CLOSE, 
                _('Input usb bus number'))
            md.run()
            md.destroy()
            return

        wl = {name:contents}
        self.media_rules[JSON_RULE_USB_NETWORK][JSON_RULE_USB_NETWORK_WHITELIST] = wl

        dg.hide()

    def on_btn_dialog_unw_input_cancel_clicked(self, obj):
        """
        cancel button of unw input dialog
        """

        self.builder.get_object('dialog_unw_input').hide()

    def on_btn_usb_network_clicked(self, obj):
        """
        click event of usb-network whitelist
        """

        self.builder.get_object('ent_unw_name').set_text(_('USB Bus No.:'))

        dg = self.builder.get_object('dialog_unw_input')
        dg.set_title(_('usb network whitelist'))

        if JSON_RULE_USB_NETWORK in self.media_rules \
            and JSON_RULE_USB_NETWORK_WHITELIST in self.media_rules[JSON_RULE_USB_NETWORK]:

            wls = self.media_rules[JSON_RULE_USB_NETWORK][JSON_RULE_USB_NETWORK_WHITELIST]
            for name, contents in wls.items():
                if name == 'usbbus':
                    self.builder.get_object('ent_unw_contents').set_text(contents)
                    break
            else:
                self.builder.get_object('ent_unw_contents').set_text('')

        dg.vbox.get_children()[1].get_children()[0].get_children()[0].set_label(_('OK'))
        dg.vbox.get_children()[1].get_children()[0].get_children()[1].set_label(_('CANCEL'))
        dg.run()
        dg.hide()

    def on_window_main_destroy(self, obj):
        """
        destroy window main
        """

        Gtk.main_quit()

    def clear_dialog_whitelist_textview(self):
        """
        clear textview of whitelist of dialog(usb_memory|bluetooth)
        """

        buff = self.builder.get_object('textview_whitelist').get_buffer()
        si = buff.get_start_iter()
        ei = buff.get_end_iter()
        buff.delete(si, ei)

    def on_btn_bluetooth_clicked(self, obj):
        """
        click event of bluetooth whitelist
        """

        self.builder.get_object('lbl_whitelist_log').set_text(
            _('how to write: 01:23:45:ab:cd:ef'))

        dg = self.builder.get_object('dialog_whitelist')
        self.ub_btn_type = 2
        dg.set_title(_('bluetooth whitelist'))
        dg.vbox.get_children()[1].get_children()[0].get_children()[0].set_label(_('OK'))
        dg.vbox.get_children()[1].get_children()[0].get_children()[1].set_label(_('CANCEL'))

        #clear textview
        self.clear_dialog_whitelist_textview()

        #fill list in
        if JSON_RULE_MAC_ADDRESS in self.media_rules[JSON_RULE_BLUETOOTH]:
            v = '\n'.join(self.media_rules[JSON_RULE_BLUETOOTH][JSON_RULE_MAC_ADDRESS])
            buff = self.builder.get_object('textview_whitelist').get_buffer()
            ei = buff.get_end_iter()
            buff.insert(ei, v)

        dg.run()
        dg.hide()

    def on_btn_usb_memory_clicked(self, obj):
        """
        click event of usb-memory whitelist
        """

        self.builder.get_object('lbl_whitelist_log').set_text(
            _('how to write: ID_SERIAL_SHORT of udev event'))

        dg = self.builder.get_object('dialog_whitelist')
        self.ub_btn_type = 1
        dg.set_title(_('usb memory whitelist'))
        dg.vbox.get_children()[1].get_children()[0].get_children()[0].set_label(_('OK'))
        dg.vbox.get_children()[1].get_children()[0].get_children()[1].set_label(_('CANCEL'))

        #clear textview 
        self.clear_dialog_whitelist_textview()

        #fill list in
        if JSON_RULE_USB_SERIALNO in self.media_rules[JSON_RULE_USB_MEMORY]:
            v = '\n'.join(self.media_rules[JSON_RULE_USB_MEMORY][JSON_RULE_USB_SERIALNO])
            buff = self.builder.get_object('textview_whitelist').get_buffer()
            ei = buff.get_end_iter()
            buff.insert(ei, v)

        dg.run()
        dg.hide()

    def on_btn_dialog_whitelist_ok_clicked(self, obj):
        """
        ok button of whitelist dialog
        """

        dg = self.builder.get_object('dialog_whitelist')

        #which media title
        title = dg.get_title()

        #get text
        buff = self.builder.get_object('textview_whitelist').get_buffer()
        si = buff.get_start_iter()
        ei = buff.get_end_iter()
        v = buff.get_text(si, ei, True)

        validate_res = VALIDATE_OK

        if self.ub_btn_type == 1:
            #validate text

            self.media_rules[JSON_RULE_USB_MEMORY][JSON_RULE_USB_SERIALNO] = \
                v.strip('\n').split('\n')
            dg.hide()

        elif self.ub_btn_type == 2:
            #validate text
            validate_res = self.validate_whitelist_bluetooth(v)
            if validate_res == VALIDATE_OK:
                self.media_rules[JSON_RULE_BLUETOOTH][JSON_RULE_MAC_ADDRESS] = \
                    v.strip('\n').split('\n')
                dg.hide()
            else:
                lbl_whitelist_log = self.builder.get_object('lbl_whitelist_log')
                lbl_whitelist_log.set_text(validate_res)

        if validate_res == VALIDATE_OK:
            #clear textview
            self.clear_dialog_whitelist_textview()

    def on_btn_dialog_whitelist_cancel_clicked(self, obj):
        """
        cancel button of whitelist dialog
        """

        #clear textview
        self.clear_dialog_whitelist_textview()
        self.builder.get_object('dialog_whitelist').hide()

        title = self.builder.get_object('dialog_whitelist').get_title()

    def on_btn_network_del_clicked(self, obj):
        """
        del button of network page
        """

        treeview_network = self.builder.get_object('treeview_network')
        list_store, list_store_iter = treeview_network.get_selection().get_selected()
        if list_store_iter:
            list_store.remove(list_store_iter)

    def on_btn_network_edit_clicked(self, obj):
        """
        edit button of network page
        """

        self.builder.get_object('lbl_network_log').set_text('')

        treeview_network = self.builder.get_object('treeview_network')
        list_store, list_store_iter = treeview_network.get_selection().get_selected()
        if list_store_iter:
            direction = list_store.get_value(list_store_iter, 0)
            proto = list_store.get_value(list_store_iter, 1)
            address = list_store.get_value(list_store_iter, 2)
            src_port = list_store.get_value(list_store_iter, 3)
            dst_port = list_store.get_value(list_store_iter, 4)
            state = list_store.get_value(list_store_iter, 5)

            cb_direction = self.builder.get_object('cb_direction')
            liststore_direction = self.builder.get_object('liststore_direction')
            liststore_direction.clear()
            liststore_direction.append(['INPUT'])
            liststore_direction.append(['OUTPUT'])
            liststore_direction.append(['ALL'])
            if direction == 'INPUT':
                cb_direction.set_active(0)
            elif direction == 'OUTPUT':
                cb_direction.set_active(1)
            else:
                cb_direction.set_active(2)

            cb_protocol = self.builder.get_object('cb_protocol')
            liststore_protocol = self.builder.get_object('liststore_protocol')
            liststore_protocol.clear()
            liststore_protocol.append(['TCP'])
            liststore_protocol.append(['UDP'])
            liststore_protocol.append(['ICMP'])
            if proto == 'TCP':
                cb_protocol.set_active(0)
            elif proto == 'UDP':
                cb_protocol.set_active(1)
            else:
                cb_protocol.set_active(2)

            ent_address = self.builder.get_object('ent_address')
            ent_address.set_text(address)

            ent_src_port = self.builder.get_object('ent_src_port')
            ent_src_port.set_text(src_port)

            ent_dst_port = self.builder.get_object('ent_dst_port')
            ent_dst_port.set_text(dst_port)

            cb_state = self.builder.get_object('cb_state')
            liststore_state = self.builder.get_object('liststore_state')
            liststore_state.clear()
            liststore_state.append(['ACCEPT'])
            liststore_state.append(['DROP'])
            if state == 'ACCEPT':
                cb_state.set_active(0)
            else:
                cb_state.set_active(1)

            dg = self.builder.get_object('dialog_network_add')
            self.ne_btn_type = 2
            dg.set_title(_('network edit'))
            dg.vbox.get_children()[2].get_children()[0].get_children()[0].set_label(_('OK'))
            dg.vbox.get_children()[2].get_children()[0].get_children()[1].set_label(_('CANCEL'))
            dg.run()
            dg.hide()

    def on_btn_dialog_network_add_ok_clicked(self, obj):
        """
        ok button of network_add dialog
        """

        dg = self.builder.get_object('dialog_network_add')
        title = dg.get_title()

        cb_direction = self.builder.get_object('cb_direction')
        liststore_direction = self.builder.get_object('liststore_direction')
        cb_protocol = self.builder.get_object('cb_protocol')
        liststore_protocol = self.builder.get_object('liststore_protocol')
        ent_address = self.builder.get_object('ent_address')
        ent_src_port = self.builder.get_object('ent_src_port')
        ent_dst_port = self.builder.get_object('ent_dst_port')
        cb_state = self.builder.get_object('cb_state')
        liststore_state = self.builder.get_object('liststore_state')

        direction = liststore_direction.get_value(cb_direction.get_active_iter(), 0)
        proto = liststore_protocol.get_value(cb_protocol.get_active_iter(), 0)
        address = ent_address.get_text()
        src_port = ent_src_port.get_text()
        dst_port = ent_dst_port.get_text()
        state = liststore_state.get_value(cb_state.get_active_iter(), 0)

        val_res = self.validate_network(address, src_port, dst_port)
        if val_res != VALIDATE_OK:
            lbl_network_log = self.builder.get_object('lbl_network_log')
            lbl_network_log.set_text(val_res)
        else:
            if self.ne_btn_type == 2:
                treeview_network = self.builder.get_object('treeview_network')
                list_store, list_store_iter = treeview_network.get_selection().get_selected()
                list_store.set_value(list_store_iter, 0, direction)
                list_store.set_value(list_store_iter, 1, proto)
                list_store.set_value(list_store_iter, 2, address)
                list_store.set_value(list_store_iter, 3, src_port)
                list_store.set_value(list_store_iter, 4, dst_port)
                list_store.set_value(list_store_iter, 5, state)
            else:
                liststore_network = self.builder.get_object('liststore_network')
                liststore_network.append([direction, proto, address, src_port, dst_port, state])

            dg.hide()

    def on_btn_dialog_network_add_cancel_clicked(self, obj):
        """
        cancel button of network_add dialog
        """

        self.builder.get_object('dialog_network_add').hide()

    def validate_whitelist_bluetooth(self, txt):
        """
        validate bluetooth of whitelist
        """

        if not txt:
            return VALIDATE_OK

        err_msg = _('invalid mac format(ex 01:23:45:ab:cd:ef)')
        macs = [t.strip() for t in txt.strip('\n').split('\n')]
        for mac in macs:
            s_mac = mac.split(':')
            if len(s_mac) != 6:
                return err_msg #+ ':mac items != 6'
            for m in s_mac:
                if len(m) != 2:
                    return err_msg #+ ':len(mac) != 2'
                m0 = ord(m[0].lower())
                m1 = ord(m[1].lower())
                for m in (m0, m1):
                    if (m >= ord('0') and m <= ord('9')) or \
                        (m >= ord('a') and m <= ord('f')):
                        pass
                    else:
                        return err_msg #+ ':wrong character'

        return VALIDATE_OK

    def ip_type(self, ip):
        """
        check ipv4 | ipv6 | domain
        """

        try:
            it = ipaddress.ip_address(ip)
            if isinstance(it, ipaddress.IPv4Address):
                return 'v4'
            return 'v6'
        except:
            return 'domain'

    def validate_network(self, address, src_port, dst_port):
        """
        validate address and ports
        """

        if not any((address, src_port, dst_port)):
            return _('need address or port')

        '''
        if self.ip_type(address) == 'domain':
            #once domain is considred as success
            pass
        else:
            #validate ip
            ip = address
            err_msg = 'invalid ip format'

            ip_items = [i.strip() for i in ip.split('.')]
            if len(ip_items) != 4:
                return err_msg + ':items != 4'

            try:
                for item in ip_items:
                    int_item = int(item)
                    if int_item < 0 or int_item > 255:
                        return err_msg + ':item<0 or item>255'
            except:
                return err_msg + ':maybe items not digit'
        '''

        #validate port
        err_msg = 'invalid port format(1-65535)'

        src_port_items = []
        dst_port_items = []
        if src_port:
            src_port_items = [i.strip() for i in src_port.split(',')]
        if dst_port:
            dst_port_items = [i.strip() for i in dst_port.split(',')]

        for port_items in (src_port_items, dst_port_items):
            try:
                for item in port_items:
                    if '-' in item:
                        left, right = [i.strip() for i in item.split('-')]
                        int_left = int(left)
                        int_right = int(right)
                        if int_left < 1 or int_left >= 65536 or int_right < 1 or int_right >= 65536:
                            return err_msg #+ ':ranged-item<1 or ranged-item>65536'
                    else:
                        int_item  = int(item)
                        if int_item < 1 or int_item >= 65536:
                            return err_msg #+ ':item<1 or item>65536'
            except:
                return err_msg #+ ':maybe items not digit or wrong ranged format'
                
        return VALIDATE_OK

    def on_btn_network_add_clicked(self, obj):
        """
        add button of network page
        """

        self.builder.get_object('lbl_network_log').set_text('')

        ent_address = self.builder.get_object('ent_address').set_text('')
        ent_src_port = self.builder.get_object('ent_src_port').set_text('')
        ent_dst_port = self.builder.get_object('ent_dst_port').set_text('')

        liststore_direction = self.builder.get_object('liststore_direction')
        liststore_direction.clear()
        liststore_direction.append(['INPUT'])
        liststore_direction.append(['OUTPUT'])
        liststore_direction.append(['ALL'])
        cb_direction = self.builder.get_object('cb_direction').set_active(0)

        liststore_protocol = self.builder.get_object('liststore_protocol')
        liststore_protocol.clear()
        liststore_protocol.append(['TCP'])
        liststore_protocol.append(['UDP'])
        liststore_protocol.append(['ICMP'])
        cb_protocol = self.builder.get_object('cb_protocol').set_active(0)

        liststore_state = self.builder.get_object('liststore_state')
        liststore_state.clear()
        liststore_state.append(['ACCEPT'])
        liststore_state.append(['DROP'])
        cb_state = self.builder.get_object('cb_state').set_active(0)

        dg = self.builder.get_object('dialog_network_add')
        self.ne_btn_type = 1
        dg.set_title(_('network add'))
        dg.vbox.get_children()[2].get_children()[0].get_children()[0].set_label(_('OK'))
        dg.vbox.get_children()[2].get_children()[0].get_children()[1].set_label(_('CANCEL'))
        dg.run()
        dg.hide()

    def on_menu_remove_user_rule_activate(self, obj):
        """
        menu > File > remove user rule
        """

        USER_RULE_PATH = '/etc/gooroom/grac.d/user.rules'
        if os.path.exists(USER_RULE_PATH):
            os.remove(USER_RULE_PATH)
        self.logging(_('removing success'))

        try:
            system_bus = dbus.SystemBus()
            bus_object = system_bus.get_object(DBUS_NAME, DBUS_OBJ)
            bus_interface = dbus.Interface(bus_object, dbus_interface=DBUS_IFACE)
            bus_interface.reload('')
            self.logging(_('apply success'))
        except:
            self.logging(traceback.format_exc())
            self.logging(_('apply fail'))

    def on_menu_apply_activate(self, obj):
        """
        menu > File > apply
        """

        self.on_menu_save_activate(obj)

        try:
            system_bus = dbus.SystemBus()
            bus_object = system_bus.get_object(DBUS_NAME, DBUS_OBJ)
            bus_interface = dbus.Interface(bus_object, dbus_interface=DBUS_IFACE)
            bus_interface.reload('')
            self.logging(_('apply success'))
        except:
            self.logging(traceback.format_exc())
            self.logging(_('save fail'))

    def on_menu_quit_activate(self, obj):
        """
        menu > File > quit
        """

        Gtk.main_quit()

    def on_menu_about_activate(self, obj):
        """
        menu > About > about
        """

        dg = self.builder.get_object('dialog_about')
        dg.set_title(_('About'))
        dg.vbox.get_children()[0].get_children()[4].set_label(_('OK'))
        dg.run()
        dg.hide()

    def on_menu_help_activate(self, obj):
        """
        menu > Help > Help
        """
        os.system("yelp help:grac-editors")

    def on_menu_load_activate(self, obj):
        """
        menu > File > load
        """

        try:
            self.media_rules = self.load_rules(DEFAULT_RULES_PATH)
            self.draw_media(self.media_rules)
            self.draw_network(self.media_rules)
            self.logging(_('reset success'))
        except:
            self.logging(traceback.format_exc())
            self.logging(_('reset fail'))
        
    def on_btn_about_ok_clicked(self, obj):
        """
        ok button of About dialog
        """

        dg = self.builder.get_object('dialog_about')
        dg.hide()

    def on_cb_protocol_changed(self, obj):
        """
        selecting protocol in network diallog
        """

        idx = obj.get_active()
        if idx < 0:
            return
        if obj.get_model()[idx][0].lower() == 'icmp':
            sp = self.builder.get_object('ent_src_port').set_sensitive(False)
            dp = self.builder.get_object('ent_dst_port').set_sensitive(False)
        else:
            sp = self.builder.get_object('ent_src_port').set_sensitive(True)
            dp = self.builder.get_object('ent_dst_port').set_sensitive(True)

    def on_rdo_usb_memory_disallow_toggled(self, obj):
        """
        usb-memory-disallow radiobutton toggled
        """

        if self.builder.get_object('rdo+usb_memory+allow').get_active():
            self.builder.get_object('btn_usb_memory').set_sensitive(False)
        else:
            self.builder.get_object('btn_usb_memory').set_sensitive(True)

    def on_rdo_usb_memory_readonly_toggled(self, obj):
        """
        usb-memory-readonly radiobutton toggled
        """

        if self.builder.get_object('rdo+usb_memory+allow').get_active():
            self.builder.get_object('btn_usb_memory').set_sensitive(False)
        else:
            self.builder.get_object('btn_usb_memory').set_sensitive(True)

    def on_rdo_bluetooth_disallow_toggled(self, obj):
        """
        bluetooth-disallow readiobutton toggled
        """

        if self.builder.get_object('rdo+bluetooth+allow').get_active():
            self.builder.get_object('btn_bluetooth').set_sensitive(False)
        else:
            self.builder.get_object('btn_bluetooth').set_sensitive(True)

    def on_rdo_usb_network_disallow_toggled(self, obj):
        """
        usb-network-disallow readiobutton toggled
        """

        if self.builder.get_object('rdo+usb_network+allow').get_active():
            self.builder.get_object('btn_usb_network').set_sensitive(False)
        else:
            self.builder.get_object('btn_usb_network').set_sensitive(True)

    def on_menu_save_activate(self, obj):
        """
        menu > File > save
        """

        try:
            ###set current media values in media_rules of memory
            grid_media = self.builder.get_object('grid_media')
            radios = [w for w in grid_media.get_children() if isinstance(w, Gtk.RadioButton)]
            for r in radios:
                if r.get_active():
                    widgetid = Gtk.Buildable.get_name(r)
                    v = widgetid.split('+')
                    mediaid = v[1]
                    state = v[2]
                    if not mediaid in self.media_rules:
                        continue

                    if isinstance(self.media_rules[mediaid], list):
                        etc_v = None
                        for tk, tv in self.media_rules[mediaid][0].items():
                            etc_v = tv
                            break
                        if not etc_v:
                            continue
                        etc_v['state'] = state
                        vid = self.builder.get_object('ent_vendor').get_text()
                        pid = self.builder.get_object('ent_product').get_text()
                        etc_v['items'][0]['vid'] = vid
                        etc_v['items'][0]['pid'] = pid
                    else:
                        self.media_rules[mediaid][JSON_RULE_STATE] = state

                    '''
                    #microphone exception
                    if mediaid == 'sound' and 'microphone' in self.media_rules:
                        self.media_rules['microphone'][JSON_RULE_STATE] = state
                    '''

            ###whitelist is modified as pushing ok button of whitelist dialog

            ###network
            network = self.media_rules[JSON_RULE_NETWORK]

            #policy
            policy = None
            if self.builder.get_object('rdo_policy_accept').get_active():
                policy = JSON_RULE_ALLOW
            else:
                policy = JSON_RULE_DISALLOW
            network[JSON_RULE_NETWORK_STATE] = policy

            #server version
            if JSON_RULE_NETWORK_VERSION in network:
                server_version = network[JSON_RULE_NETWORK_VERSION]
            else:
                server_version = '1.0'

            #iptables
            liststore_network = self.builder.get_object('liststore_network')
            new_network_rules = []
            disassemble_rules = {}
            for ln in liststore_network:
                #SERVER VERSION 1.0
                if server_version.startswith('1.0'):
                    direction = ln[0]
                    address = ln[2]
                    state = ln[5]

                    proto = ln[1]
                    src_port = ln[3]
                    dst_port = ln[4]
                    
                    if (direction,address,state) in disassemble_rules:
                        disassemble_rules[(direction,address,state)].append((proto, src_port, dst_port))
                    else:
                        disassemble_rules[(direction,address,state)] = [(proto, src_port, dst_port)]
                #SERVER NOT VERSION 1.0
                else:
                    direction = ln[0]
                    address = ln[2]
                    state = ln[5]

                    proto = ln[1]
                    src_port = ln[3]
                    dst_port = ln[4]
                    
                    rule = {}
                    rule[JSON_RULE_NETWORK_DIRECTION] = direction.lower()
                    rule[JSON_RULE_NETWORK_ADDRESS] = address
                    rule[JSON_RULE_NETWORK_STATE] = state.lower()
                    rule[JSON_RULE_NETWORK_PROTO] = proto.lower()
                    rule[JSON_RULE_NETWORK_SRC_PORTS] = src_port
                    rule[JSON_RULE_NETWORK_DST_PORTS] = dst_port
                    new_network_rules.append(rule)

            #SERVER VERSION 1.0
            if server_version.startswith('1.0'):
                for das, psd_list in disassemble_rules.items():
                    rule = {}
                    direction, address, state = das
                    rule[JSON_RULE_NETWORK_DIRECTION] = direction
                    rule[JSON_RULE_NETWORK_ADDRESS] = address

                    if state == 'ACCEPT':
                        state = JSON_RULE_ALLOW
                    else:
                        state = JSON_RULE_DISALLOW

                    rule[JSON_RULE_NETWORK_STATE] = state

                    if psd_list and len(psd_list) > 0:
                        rule[JSON_RULE_NETWORK_PORTS] = []
                        for psd in psd_list:
                            proto, src_port, dst_port = psd
                            if proto or src_port or dst_port:
                                rule[JSON_RULE_NETWORK_PORTS].append({
                                                JSON_RULE_NETWORK_SRC_PORT:src_port.split(','),
                                                JSON_RULE_NETWORK_PROTO:proto,
                                                JSON_RULE_NETWORK_DST_PORT:dst_port.split(',')})

                    new_network_rules.append(rule)

            if len(new_network_rules) > 0:
                network[JSON_RULE_NETWORK_RULE] = new_network_rules
            else:
                if JSON_RULE_NETWORK_RULE in self.media_rules[JSON_RULE_NETWORK]:
                    del network[JSON_RULE_NETWORK_RULE]

            ###transform editor structure to orginal's
            org_form = {}
            for k, v in self.media_rules.items():
                if len(v) == 1 and not isinstance(v, list):
                    org_form[k] = v[JSON_RULE_STATE]
                else:
                    org_form[k] = v
            
            ###save to file
            TMP_MEDIA_DEFAULT = '/var/tmp/TMP-MEDIA-DEFAULT'
            with open(TMP_MEDIA_DEFAULT, 'w') as f:
                f.write(json.dumps(org_form, indent=4, ensure_ascii=False))

            shutil.copy(TMP_MEDIA_DEFAULT, DEFAULT_RULES_PATH)

            self.logging(_('save success'))
        except:
            self.logging(traceback.format_exc())

    def lang(self):
        """
        under international flag
        """

        #main
        self.builder.get_object('window_main').set_title(_('GRAC Editor'))

        #media
        self.builder.get_object('rdo+cd_dvd+allow').set_label(_('allow'))
        self.builder.get_object('rdo+printer+allow').set_label(_('allow'))
        self.builder.get_object('rdo+wireless+allow').set_label(_('allow'))
        self.builder.get_object('rdo+camera+allow').set_label(_('allow'))
        self.builder.get_object('rdo+keyboard+allow').set_label(_('allow'))
        self.builder.get_object('rdo+sound+allow').set_label(_('allow'))
        self.builder.get_object('rdo+mouse+allow').set_label(_('allow'))
        self.builder.get_object('rdo+bluetooth+allow').set_label(_('allow'))
        self.builder.get_object('rdo+microphone+allow').set_label(_('allow'))
        self.builder.get_object('rdo+usb_memory+allow').set_label(_('allow'))
        self.builder.get_object('rdo+screen_capture+allow').set_label(_('allow'))
        self.builder.get_object('rdo+clipboard+allow').set_label(_('allow'))
        self.builder.get_object('rdo+usb_etc+allow').set_label(_('allow'))

        self.builder.get_object('rdo+cd_dvd+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+printer+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+wireless+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+camera+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+keyboard+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+sound+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+mouse+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+bluetooth+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+microphone+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+usb_memory+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+usb_memory+read_only').set_label(_('read only'))
        self.builder.get_object('rdo+screen_capture+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+clipboard+disallow').set_label(_('disallow'))
        self.builder.get_object('rdo+usb_etc+disallow').set_label(_('disallow'))

        self.builder.get_object('lbl_usb_memory').set_label(_('USB Memory'))
        self.builder.get_object('lbl_printer').set_label(_('Printer'))
        self.builder.get_object('lbl_cd_dvd').set_label(_('CD/DVD'))
        self.builder.get_object('lbl_camera').set_label(_('Camera'))
        self.builder.get_object('lbl_sound').set_label(_('Sound'))
        self.builder.get_object('lbl_microphone').set_label(_('Microphone'))
        self.builder.get_object('lbl_wireless').set_label(_('Wireless'))
        self.builder.get_object('lbl_bluetooth').set_label(_('Bluetooth'))
        self.builder.get_object('lbl_keyboard').set_label(_('USB Keyboard'))
        self.builder.get_object('lbl_mouse').set_label(_('USB Mouse'))
        self.builder.get_object('lbl_screen_capture').set_label(_('ScreenCapture'))
        self.builder.get_object('lbl_clipboard').set_label(_('Clipboard'))
        self.builder.get_object('lbl_usb_etc').set_label(_('USB ETC'))

        self.builder.get_object('btn_usb_memory').set_label(_('WL'))
        self.builder.get_object('btn_bluetooth').set_label(_('WL'))

        #notebook
        self.builder.get_object('lbl_media').set_label(_('media'))
        self.builder.get_object('lbl_network').set_label(_('network'))

        #menu
        self.builder.get_object('menuitem1').set_label(_('_File(F)'))
        self.builder.get_object('menuitem4').set_label(_('_Help(H)'))
        self.builder.get_object('menu_load').set_label(_('_Reset(R)'))
        self.builder.get_object('menu_apply').set_label(_('_Save(S)'))
        self.builder.get_object('menu_quit').set_label(_('_Quit(Q)'))
        self.builder.get_object('menu_about').set_label(_('_About(A)'))
        self.builder.get_object('menu_help').set_label(_('_Help'))
        self.builder.get_object('menu_remove_user_rule').set_label(_('_Remove User Rule(U)'))

        #network
        self.builder.get_object('lbl_policy').set_label(_('POLICY'))
        self.builder.get_object('rdo_policy_accept').set_label(_('accept'))
        self.builder.get_object('rdo_policy_drop').set_label(_('drop'))

        self.builder.get_object('btn_network_add').set_label(_('add'))
        self.builder.get_object('btn_network_del').set_label(_('del'))
        self.builder.get_object('btn_network_edit').set_label(_('edit'))

        #about
        self.builder.get_object('lbl_about').set_label(_('Gooroom Platform\n\nGRAC Editor 1.0.0'))
        
        #toolbar
        self.builder.get_object('toolbar_reload').set_label(_('reset'))
        self.builder.get_object('toolbar_apply').set_label(_('save'))

        #event
        self.builder.get_object('lbl_log').set_label(_('event log'))

        #usb network
        self.builder.get_object('btn_usb_network').set_label(_('WL'))
        self.builder.get_object('rdo+usb_network+allow').set_label(_('allow'))
        self.builder.get_object('rdo+usb_network+disallow').set_label(_('disallow'))
        self.builder.get_object('lbl_usb_network').set_label(_('USB Network'))
        self.builder.get_object('btn_dialog_usb_network_whitelist_add').set_label(_('add'))
        self.builder.get_object('btn_dialog_usb_network_whitelist_del').set_label(_('del'))
        self.builder.get_object('btn_dialog_usb_network_whitelist_edit').set_label(_('edit'))

        #padding inactive
        self.builder.get_object('mi_padding').set_sensitive(False)
        self.builder.get_object('tb_padding').set_sensitive(False)

#-----------------------------------------------------------------------
def check_online_account():
    """
    check if login-account is online-account
    """

    import struct
    from pwd import getpwnam

    with open('/var/run/utmp', 'rb') as f:
        fc = memoryview(f.read())

    utmp_fmt = '<ii32s4s32s'
    user_id = '-'

    for i in range(int(len(fc)/384)):
        ut_type, ut_pid, ut_line, ut_id, ut_user = \
            struct.unpack(utmp_fmt, fc[384*i:76+(384*i)])
        ut_line = ut_line.decode('utf8').strip('\00')
        ut_id = ut_id.decode('utf8').strip('\00')

        if ut_type == 7 and ut_id == ':0':
            user_id = ut_user.decode('utf8').strip('\00')

    #check if user_id is an online account
    with open('/etc/passwd') as f:
        pws = f.readlines()

    if user_id == '-':
        return False, 'can not find login-account.'

    #user_id is a local account
    gecos = getpwnam(user_id).pw_gecos.split(',')
    if len(gecos) >= 5 and gecos[4] == 'gooroom-account':
        return False, _('online-account can not use editor.')
    else:
        user_id = '+' + user_id
        return True, ''

#-------------------------------------------------------------------------------
if __name__ == '__main__':

    res, msg = check_online_account()
    if not res:
        md = Gtk.MessageDialog(
            None, 
            0, 
            Gtk.MessageType.WARNING, 
            Gtk.ButtonsType.CLOSE, 
            msg)
        md.run()
        md.destroy()
        sys.exit(1)

    ge = GracEditor()
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    Gtk.main()


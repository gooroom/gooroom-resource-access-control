#!/usr/bin/env python3

#-------------------------------------------------------------------------------
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import simplejson as json
import traceback
import signal
import dbus
import sys

from ge_define import *

#-------------------------------------------------------------------------------
class GracEditor:
    """
    GRAC EDITOR
    """

    def __init__(self):

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

    def logging(self, txt):
        """
        write on textview_log
        """

        buff = self.builder.get_object('textview_log').get_buffer()
        si = buff.get_start_iter()
        ei = buff.get_end_iter()
        buff.delete(si, ei)

        ei = buff.get_end_iter()
        buff.insert(ei, txt)

    def draw_network(self, rules):
        """
        setting network page as values of rules file
        """

        network_rule = rules[JSON_RULE_NETWORK]

        #set policy
        policy = network_rule[JSON_RULE_STATE]
        if policy == JSON_RULE_ALLOW:
            policy = NETWORK_ACCEPT
        else:
            policy = NETWORK_DROP

        policy_cb = self.builder.get_object('rdo_policy_{}'.format(policy))
        policy_cb.set_active(True)

        #set treeview
        treeview = self.builder.get_object('treeview_network')

        if not JSON_RULE_NETWORK_RULE in network_rule:
            return

        rules = network_rule[JSON_RULE_NETWORK_RULE]
        if not rules or len(rules) <= 0:
            return

        liststore_network = self.builder.get_object('liststore_network')
        liststore_network.clear()

        #sort by address
        cell_renderer = Gtk.CellRendererText()
        col = Gtk.TreeViewColumn('ADDRESS', cell_renderer)
        col.set_sort_column_id(2)

        for rule in rules:
            address = rule[JSON_RULE_NETWORK_ADDRESS]
            state = rule[JSON_RULE_NETWORK_STATE]
            if state == JSON_RULE_ALLOW:
                state = NETWORK_ACCEPT
            else:
                state = NETWORK_DROP
            state = state.upper()

            direction = rule[JSON_RULE_NETWORK_DIRECTION].upper()
            ports = rule[JSON_RULE_NETWORK_PORTS]

            if not JSON_RULE_NETWORK_PORTS in rule:
                liststore_network.append([direction, '', address, '', '', state])
                continue

            for port in ports:
                src_port = dst_port = ''
                if JSON_RULE_NETWORK_SRC_PORT in port:
                    src_port = ','.join(port[JSON_RULE_NETWORK_SRC_PORT])
                if JSON_RULE_NETWORK_DST_PORT in port:
                    dst_port = ','.join(port[JSON_RULE_NETWORK_DST_PORT])
                proto = port[JSON_RULE_NETWORK_PROTO].upper()
                liststore_network.append([direction, proto, address, src_port, dst_port, state])

    def draw_media(self, rules):
        """
        setting media page as the values of rules file
        """
    
        for k, v in rules.items():
            obj_name = 'rdo+{}+{}'.format(k, v['state'])
            cb = self.builder.get_object(obj_name)
            if cb:
                cb.set_active(True)
            else:
                pass

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
            else:
                new_json_rules[k] = {}
                for k2, v2 in v.items():
                    if k2 == JSON_RULE_MAC:
                        new_json_rules[k][JSON_RULE_WHITELIST] = v2
                    else:
                        new_json_rules[k][k2] = v2

        return new_json_rules

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

        self.builder.get_object('lbl_whitelist_log').set_text('')

        dg = self.builder.get_object('dialog_whitelist')
        dg.set_title(TITLE_WL_BLUETOOTH)

        #clear textview
        self.clear_dialog_whitelist_textview()

        #fill list in
        if JSON_RULE_WHITELIST in self.media_rules[JSON_RULE_BLUETOOTH]:
            v = '\n'.join(self.media_rules[JSON_RULE_BLUETOOTH][JSON_RULE_WHITELIST])
            buff = self.builder.get_object('textview_whitelist').get_buffer()
            ei = buff.get_end_iter()
            buff.insert(ei, v)

        dg.run()
        dg.hide()

    def on_btn_usb_memory_clicked(self, obj):
        """
        click event of usb-memory whitelist
        """

        self.builder.get_object('lbl_whitelist_log').set_text('')

        dg = self.builder.get_object('dialog_whitelist')
        dg.set_title(TITLE_WL_USB_MEMORY)

        #clear textview 
        self.clear_dialog_whitelist_textview()

        #fill list in
        if JSON_RULE_WHITELIST in self.media_rules[JSON_RULE_USB_MEMORY]:
            v = '\n'.join(self.media_rules[JSON_RULE_USB_MEMORY][JSON_RULE_WHITELIST])
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

        if title == TITLE_WL_USB_MEMORY:
            #validate text

            self.media_rules[JSON_RULE_USB_MEMORY][JSON_RULE_WHITELIST] = \
                v.strip('\n').split('\n')
        elif title == TITLE_WL_BLUETOOTH:
            #validate text
            validate_res = self.validate_whitelist_bluetooth(v)
            if validate_res == VALIDATE_OK:
                self.media_rules[JSON_RULE_BLUETOOTH][JSON_RULE_WHITELIST] = \
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
            dg.set_title(TITLE_NETWORK_EDIT)
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
            if title == TITLE_NETWORK_EDIT:
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

        err_msg = 'invalid mac'
        macs = [t.strip() for t in txt.strip('\n').split('\n')]
        for mac in macs:
            s_mac = mac.split(':')
            if len(s_mac) != 6:
                return err_msg + ':mac items != 6'
            for m in s_mac:
                if len(m) != 2:
                    return err_msg + ':len(mac) != 2'
                m0 = ord(m[0].lower())
                m1 = ord(m[1].lower())
                for m in (m0, m1):
                    if (m >= ord('0') and m <= ord('9')) or \
                        (m >= ord('a') and m <= ord('f')):
                        pass
                    else:
                        return err_msg + ':wrong character'

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

        if self.ip_type == 'domain':
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

        #validate port
        err_msg = 'invalid port format'

        src_port_items = [i.strip() for i in src_port.split(',')]
        dst_port_items = [i.strip() for i in dst_port.split(',')]
        for port_items in (src_port_items, dst_port_items):
            try:
                for item in port_items:
                    if '-' in item:
                        left, right = [i.strip() for i in item.split('-')]
                        int_left = int(left)
                        int_right = int(right)
                        if int_left < 1 or int_left > 65536 or int_right < 1 or int_right > 65536:
                            return err_msg + ':ranged-item<1 or ranged-item>65536'
                    else:
                        int_item  = int(item)
                        if int_item < 1 or int_item > 65536:
                            return err_msg + ':item<1 or item>65536'
            except:
                return err_msg + ':maybe items not digit or wrong ranged format'
                
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
        dg.set_title(TITLE_NETWORK_ADD)
        dg.run()
        dg.hide()

    def on_menu_apply_activate(self, obj):
        """
        menu > File > apply
        """

        try:
            system_bus = dbus.SystemBus()
            bus_object = system_bus.get_object(DBUS_NAME, DBUS_OBJ)
            bus_interface = dbus.Interface(bus_object, dbus_interface=DBUS_IFACE)
            bus_interface.reload('')
            self.logging('apply success')
        except:
            self.logging(traceback.format_exc())

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
        dg.set_title('About')
        dg.run()
        dg.hide()

    def on_menu_load_activate(self, obj):
        """
        menu > File > load
        """

        try:
            self.draw_media(self.media_rules)
            self.draw_network(self.media_rules)
            self.logging('load success')
        except:
            self.logging(traceback.format_exc())
        
    def on_btn_about_ok_clicked(self, obj):
        """
        ok button of About dialog
        """

        dg = self.builder.get_object('dialog_about')
        dg.hide()

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
                    self.media_rules[mediaid][JSON_RULE_STATE] = state

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

            #iptables
            liststore_network = self.builder.get_object('liststore_network')
            disassemble_rules = {}
            for ln in liststore_network:
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
                    
            new_network_rules = []
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
                if len(v) == 1:
                    org_form[k] = v[JSON_RULE_STATE]
                else:
                    org_form[k] = v
            
            ###save to file
            with open(DEFAULT_RULES_PATH, 'w') as f:
                f.write(json.dumps(org_form, indent=4))

            self.logging('save success')
        except:
            self.logging(traceback.format_exc())

#-------------------------------------------------------------------------------
if __name__ == '__main__':

    ge = GracEditor()
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    Gtk.main()


#! /usr/bin/env python3

#-----------------------------------------------------------------------
import simplejson as json
import threading
import psutil
import shutil
import dbus
import pwd
import os

from grac_util import GracLog,GracConfig,grac_format_exc,catch_user_id
from grac_define import *

#-----------------------------------------------------------------------
LSF_REGISTERED = 'GOOROOM-LIGHTWEIGHT-SECURITY-FRAMEWORK'

#-----------------------------------------------------------------------
class GracApi:
    """
    GRAC API
    """

    def __init__(self, grac):

        self.GRAC = grac

        self.logger = GracLog.get_logger()

        self.api_list = [
            'lsf_media_get_state',
            'lsf_media_set_state',
            'lsf_media_get_whitelist',
            'lsf_media_set_whitelist',
            'lsf_media_append_whitelist',
            'lsf_media_remove_whitelist'
        ]

        self.default_rules_path = '/etc/gooroom/grac.d/default.rules'
        self.regi_path = '/etc/gooroom/sdk/media/regi.d/'

    def make_response(self, msg, status, errmsg, **kargs):
        """
        make response message
        """

        letter = msg['letter']

        if 'parameters' in letter:
            del letter['parameters'] 

        letter['return'] = {}
        letter['return']['status'] = status

        for k, v in kargs.items():
            letter['return'][k] = v

        if errmsg:
            letter['return']['errmsg'] = errmsg

    def get_username_with_uid(self, pid):
        """
        get username with uid of the process that have called api
        """

        try:
            with open('/proc/{}/status'.format(pid)) as f:
                for l in f.readlines():
                    if l.startswith('Uid:'):
                        uid = int(l.split()[1])
                        return pwd.getpwuid(uid).pw_name
        except:
            self.logger.error(grac_format_exc())

        return None

    def get_login_username(self):
        """
        get login username
        """

        try:
            login_id, _ = catch_user_id();

            if login_id and login_id[0] == '-':
                login_id = None
        except:
            login_id = None
            self.logger.error(grac_format_exc())

        return login_id

    def check_auth(self, sender):
        """
        check authority of sender
        """

        bus_object = dbus.SystemBus().get_object(
            'org.freedesktop.DBus', '/org/freedesktop/DBus')
        bus_interface = dbus.Interface(
            bus_object, dbus_interface='org.freedesktop.DBus')
        pid = bus_interface.GetConnectionUnixProcessID(sender)

        ps = psutil.Process(pid)
        exe = ps.exe().split()[0]
        cmds = ps.cmdline()

        checking_usernames = [LSF_REGISTERED]
        login_username = self.get_login_username()
        if login_username:
            if login_username[0] == '+':
                checking_usernames.append(login_username[1:])
            else:
                raise Exception('remote account not supported')

        uid_username = self.get_username_with_uid(pid)
        if uid_username and uid_username not in checking_usernames:
            checking_usernames.append(uid_username)

        for username in checking_usernames:
            if not os.path.exists(self.regi_path+username):
                continue

            with open(self.regi_path+username) as f:
                regis = [r for r in f.read().split('\n') if r]

            if exe in regis:
                return True

            cmds_str = ' '.join([exe] + cmds[1:])
            if cmds_str in regis and not os.path.exists(cmds_str):
                return True
            
        return False
        
    def do_api(self, msg, sender):
        """
        API ENTRY
        """

        try:
            if not self.check_auth(sender):
                raise Exception('no authority')

            rules = self.GRAC.data_center.get_json_rules()
            if not rules:
                raise Exception('not ready yet')

            letter = msg['letter']
            fname = letter['func_name']

            if not fname in self.api_list:
                raise Exception('unknown function name')
            
            getattr(self, fname)(msg)

        except Exception as e:
            self.logger.error(grac_format_exc())
            self.make_response(msg, API_ERROR, str(e))
            
        except: 
            e = grac_format_exc()
            self.logger.error(e)
            self.make_response(msg, API_ERROR, 'INTERNAL ERROR')

        return json.dumps(msg)

    def load_default_rules(self):
        """
        load default.rules
        """

        with open(self.default_rules_path, 'r') as f:
            return json.loads(f.read().strip('\n'))
            
    def save_default_rules(self, rules):
        """
        save default.rules
        """

        '''
        with open(self.default_rules_path, 'w') as f:
            f.write(json.dumps(rules))
        '''

        TMP_MEDIA_DEFAULT_API = '/var/tmp/TMP-MEDIA-DEFAULT-API'
        with open(TMP_MEDIA_DEFAULT_API, 'w') as f:
            f.write(json.dumps(rules, indent=4))

        shutil.copy(TMP_MEDIA_DEFAULT_API, self.default_rules_path)

    def lsf_media_get_state(self, msg):
        """
        get media state
        """

        media_name = msg['letter']['parameters']['media_name']
        rules = self.load_default_rules()
        state = rules[media_name]
        if isinstance(state, dict):
            state = state[JSON_RULE_STATE]
        self.make_response(msg, API_SUCCESS, None, **{'state':state})

    def lsf_media_set_state(self, msg):
        """
        set media state
        """

        media_name = msg['letter']['parameters']['media_name']
        media_state = msg['letter']['parameters']['media_state']
        if media_state != JSON_RULE_ALLOW \
            and media_state != JSON_RULE_DISALLOW \
            and media_state != JSON_RULE_READONLY:
            raise Exception('invalid media_state value')
            
        rules = self.load_default_rules()
        file_state = rules[media_name]
        if isinstance(file_state, dict):
            file_state[JSON_RULE_STATE] = media_state
        else:
            rules[media_name] = media_state

        self.save_default_rules(rules)
        self.I_reload()
            
        self.make_response(msg, API_SUCCESS, None)

    def get_whitelist_name(self, media_name):
        """
        get whitelist name
        """

        if media_name == JSON_RULE_USB_MEMORY:
            wl_name = 'usb_serialno'
        elif media_name == JSON_RULE_BLUETOOTH:
            wl_name = 'mac_address'
        elif media_name == JSON_RULE_USB_NETWORK:
            wl_name = 'whitelist'
        else:
            wl_name = None

        if not wl_name:
            raise Exception('{} not suported whitelist'.format(media_name))

        return wl_name

    def lsf_media_get_whitelist(self, msg):
        """
        get media whitelist
        """

        media_name = msg['letter']['parameters']['media_name']

        wl_name = self.get_whitelist_name(media_name)

        rules = self.load_default_rules()
        state = rules[media_name]
        if isinstance(state, dict) and wl_name in state:
            wl = state[wl_name]
        else:
            wl = []

        self.make_response(msg, API_SUCCESS, None, **{'whitelist':wl})

    def lsf_media_set_whitelist(self, msg):
        """
        set media whitelist
        """

        media_name = msg['letter']['parameters']['media_name']

        wl_name = self.get_whitelist_name(media_name)

        wl = msg['letter']['parameters']['whitelist']
        rules = self.load_default_rules()
        state = rules[media_name]
        if isinstance(state, dict):
            state[wl_name] = wl
        else:
            rules[media_name] = {}
            rules[media_name][JSON_RULE_STATE] = state
            rules[media_name][wl_name] = wl

        self.save_default_rules(rules)
        self.I_reload()
            
        self.make_response(msg, API_SUCCESS, None)

    def lsf_media_append_whitelist(self, msg):
        """
        append media whitelist
        """

        media_name = msg['letter']['parameters']['media_name']

        wl_name = self.get_whitelist_name(media_name)

        wl = msg['letter']['parameters']['whitelist']
        rules = self.load_default_rules()
        state = rules[media_name]
        if isinstance(state, dict):
            if wl_name in state:
                state[wl_name].extend(wl)
            else:
                rules[media_name][wl_name] = wl
                
        else:
            rules[media_name] = {}
            rules[media_name][JSON_RULE_STATE] = state
            rules[media_name][wl_name] = wl

        self.save_default_rules(rules)
        self.I_reload()
            
        self.make_response(msg, API_SUCCESS, None)

    def lsf_media_remove_whitelist(self, msg):
        """
        remove media whitelist
        """

        media_name = msg['letter']['parameters']['media_name']

        wl_name = self.get_whitelist_name(media_name)

        wl = msg['letter']['parameters']['whitelist']
        rules = self.load_default_rules()
        state = rules[media_name]
        if isinstance(state, dict) and wl_name in state:
            state[wl_name] = [w for w in state[wl_name] if not w in wl]
                
        self.save_default_rules(rules)
        self.I_reload()
            
        self.make_response(msg, API_SUCCESS, None)

    def I_reload(self):
        """
        reload myself
        """

        server_rule = '/etc/gooroom/grac.d/user.rules'
        if os.path.exists(server_rule):
            os.unlink(server_rule)
        self.GRAC.reload('from API')


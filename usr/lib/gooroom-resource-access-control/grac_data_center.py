#! /usr/bin/env python3

#-----------------------------------------------------------------------
from configobj import ConfigObj
import simplejson as json
import importlib
import threading
import glob
import sys
import re
import os

from grac_util import GracConfig,GracLog,verify_rule,grac_format_exc
from grac_util import make_media_msg,red_alert2
from grac_define import *

#-----------------------------------------------------------------------
class GracDataCenter:
    """
    DATA CENTER 
    """

    def __init__(self, grac):

        self.GRAC = grac

        self.conf = GracConfig.get_config()
        self.logger = GracLog.get_logger()

        self._center_lock = threading.Lock()

        #UDEV RULES MAP
        self._rules_map = None

        #JSON RULES
        self._json_rules = None

        #MODULES
        self._modules = None

        #ALERT TIMESTAMP
        self._alert_timestamp = None

        #SOUND PREVIOUS STATE
        self.snd_prev_state = None
        
        #MICROPHONE PREVIOUS STATE
        self.mic_prev_state = None

    def show(self):
        """
        show 
        """

        self._center_lock.acquire()

        try:
            self.logger.info('BEGIN SHOW()')

            conf = GracConfig.get_config(reload=True)
            self.alert_span = int(conf.get('MAIN', 'ALERT_SPAN'))

            self._alert_timestamp = {}
            self._rules_map = self.load_rules_map()
            self._json_rules = self.load_json_rules()
            self._rules_map = \
                self.adjust_rules_map_with_json_rules(self._rules_map, self._json_rules)
            self.load_python_modules()
            self._bluetooth_whitelist = self.pick_bluetooth_whitelist(self._json_rules)
            self._usb_memory_whitelist = self.pick_usb_memory_whitelist(self._json_rules)
            self._usb_network_whitelist = self.pick_usb_network_whitelist(self._json_rules)

            self.logger.info('END SHOW()')

        except:
            raise

        finally:
            self._center_lock.release()

    def get_rules_map(self):
        """
        get rules map
        """

        self._center_lock.acquire()

        try:
            return self._rules_map

        except:
            raise

        finally:
            self._center_lock.release()

    def load_rules_map(self):
        """
        load grac-udev-rules.map
        """

        rules_map_path = self.conf.get('MAIN', 'GRAC_RULE_MAP_PATH')
        parser = ConfigObj(rules_map_path)

        #{'usb_memory':{'disallow':{'filters':[ [(x,x,x,x), (x,x,x,x)], [(x,x,x,x)] ]}}
        rules_map = {}

        for device, device_v in parser.items():
            rules_map[device] = {}

            for  mode, mode_v in device_v.items():
                if not RULES_MAP_FILTER in mode_v:
                    self.logger.error(
                        '!! {} not found(filters={})'.format(RULES_MAP_FILTER, mode_v))
                    continue
                
                filter_chunks = mode_v[RULES_MAP_FILTER].split('\n')
                filter_list = []
            
                for filter_chunk in filter_chunks:
                    filter_chunk = filter_chunk.strip()

                    #skip empty line
                    if not filter_chunk:
                        continue
                    #skip comment
                    if filter_chunk[0] == '#':
                        continue

                    #item consists of (type, lhs, rhs, op)
                    lhs, rhs, op, tp = None, None, None, None
                    #MODULE should be in filter
                    module_mustbe = False
                    item_list = []

                    for filter_item in filter_chunk.split(','):
                        filter_item = filter_item.strip()
                        #skip inline comment 
                        if filter_item[0] == '#':
                            continue

                        #support A==B, A!=B, MODULE=xxx
                        if '==' in filter_item:
                            lhs, rhs = filter_item.split('==')
                            op = '=='
                        elif '!=' in filter_item:
                            lhs, rhs = filter_item.split('!=')
                            op = '!='
                        elif '=' in filter_item:
                            lhs, rhs = filter_item.split('=')
                            op = '='
                        else:
                            self.logger.error('!! filter_item operator '\
                                'is not valid(filter_item={})'.format(filter_item))
                            continue

                        lhs = lhs.strip()
                        rhs = rhs.strip().replace('"', '')

                        #environment, attribute, module, property
                        if lhs.startswith(RULES_MAP_TYPE_ENV):
                            tp = RULES_MAP_TYPE_ENV
                            lhs = lhs[4:-1]
                        elif lhs.startswith(RULES_MAP_TYPE_ATTRS):
                            tp = RULES_MAP_TYPE_ATTRS
                            lhs = lhs[6:-1]
                        elif lhs.startswith(RULES_MAP_TYPE_ATTR):
                            tp = RULES_MAP_TYPE_ATTR
                            lhs = lhs[5:-1]
                        elif lhs.startswith(RULES_MAP_TYPE_MODULE):
                            tp = RULES_MAP_TYPE_MODULE
                            module_mustbe = True
                        else:
                            tp = RULES_MAP_TYPE_PROP

                        #store item(A==B)
                        item_list.append((tp,lhs,rhs,op))

                    if not module_mustbe:
                        self.logger.error('!! MODULE should be '\
                            'in filter(filter={})'.format(filter_chunk))
                        continue
                    if len(item_list) <= 0:
                        self.logger.error('!! item number should be '\
                            'more than 1(filter={})'.format(filter_chunk))
                        continue

                    #store filter(A==B,A!=C,MODULE=xxx)
                    filter_list.append(item_list)

                #store filters of device with mode
                rules_map[device][mode] = {}
                rules_map[device][mode][RULES_MAP_FILTER] = filter_list

        return rules_map

    def get_module(self, module_name):
        """
        get python module
        """

        self._center_lock.acquire()

        try:
            return self._modules[module_name]

        except:
            raise

        finally:
            self._center_lock.release()

    def load_python_modules(self):
        """
        load python modules
        """

        self._modules = {}

        module_path = self.conf.get('MAIN', 'GRAC_MODULE_PATH')
        if module_path[-1] != '/':
            module_path += '/'

        #PYTHONPATH에 모듈디렉토리경로 추가
        sys.path.append(module_path)

        module_fullpath = '%s*' % module_path

        py_ext = re.compile('.*[.py|.pyc]$')

        for module_fullname in (f for f in glob.glob(module_fullpath) if py_ext.match(f)):
            module_name = module_fullname.split('/')[-1].split('.')[0]

            if module_name in self._modules:
                continue

            self._modules[module_name] = importlib.import_module(module_name)
            self.logger.info('%s(%s) loaded' % (module_name, module_fullname))

    def get_json_rules(self):
        """
        get josn rules
        """

        self._center_lock.acquire()

        try:
            return self._json_rules

        except:
            raise

        finally:
            self._center_lock.release()

    def get_media_state(self, media_name):
        """
        get media state(allow/disallow) for media_name
        """

        state = self.get_json_rules()[media_name]
        if isinstance(state, dict):
            state = state[JSON_RULE_STATE]
        return state

    def load_json_rules(self):
        """
        load user|default rules file
        """

        default_json_rules_path = self.conf.get('MAIN', 'GRAC_DEFAULT_JSON_RULES_PATH')
        user_json_rules_path = self.conf.get('MAIN', 'GRAC_USER_JSON_RULES_PATH')
        json_rules_path = None

        if os.path.exists(user_json_rules_path):
            json_rules_path = user_json_rules_path
            try:
                verify_rule(json_rules_path)
                m = 'The signature verification of the policy file was successful'
                self.logger.info('{}: {}'.format(json_rules_path, m))
                red_alert2(m, 
                    '', 
                    JLEVEL_DEFAULT_SHOW, 
                    GRMCODE_SIGNATURE_SUCCESS, 
                    self,
                    flag=RED_ALERT_JOURNALONLY)
            except:
                json_rules_path = default_json_rules_path
                self.logger.error(grac_format_exc())
                m = 'The signature verification of the policy file failed'
                self.logger.info(
                    '{}: {}: opening default'.format(json_rules_path, m))
                red_alert2(
                    m, 
                    '서명 검증 실패', 
                    JLEVEL_DEFAULT_NOTI, 
                    GRMCODE_SIGNATURE_FAIL,
                    self)
        else:
            json_rules_path = default_json_rules_path
            
        with open(json_rules_path) as f:
            json_rules = json.loads(f.read().strip('\n'))

        return json_rules

    def adjust_rules_map_with_json_rules(self, rules_map, json_rules):
        """
        rules_map에서 json_rules에 있는 항목과 모드가 일치하는 
        내용만 선택한다
        """

        new_map = {}

        for target, mod_or_dict in json_rules.items():
            if target in rules_map:
                mode = None
                if isinstance(mod_or_dict, str):
                    mode = mod_or_dict
                else:
                    mode = mod_or_dict[JSON_RULE_STATE]

                if mode in rules_map[target]:
                    new_map[target] = {}
                    new_map[target][mode] = rules_map[target][mode]
                    self.logger.info(
                        'added {}:{} to rules'.format(target, mode))
        return new_map

    def pick_bluetooth_whitelist(self, json_rules):
        """
        pick bluetooth whitelist in json rules
        """
        
        if not JSON_RULE_BLUETOOTH_MAC in json_rules[JSON_RULE_BLUETOOTH]:
            return []

        return json_rules[JSON_RULE_BLUETOOTH][JSON_RULE_BLUETOOTH_MAC]

    def pick_usb_memory_whitelist(self, json_rules):
        """
        pick usb-memory whitelist in json rules
        """
        
        if not JSON_RULE_USB_MEMORY_WHITELIST in json_rules[JSON_RULE_USB_MEMORY]:
            return []

        return json_rules[JSON_RULE_USB_MEMORY][JSON_RULE_USB_MEMORY_WHITELIST]

    def pick_usb_network_whitelist(self, json_rules):
        """
        pick usb-network whitelist in json rules
        """
        
        if not JSON_RULE_USB_NETWORK in json_rules \
            or not JSON_RULE_USB_NETWORK_WHITELIST in json_rules[JSON_RULE_USB_NETWORK]:
            return {}

        return json_rules[JSON_RULE_USB_NETWORK][JSON_RULE_USB_NETWORK_WHITELIST]

    def get_bluetooth_whitelist(self):
        """
        get bluetooth whitelist
        """

        self._center_lock.acquire()

        try:
            return self._bluetooth_whitelist

        except:
            raise

        finally:
            self._center_lock.release()

    def get_usb_memory_whitelist(self):
        """
        get usb-memory whitelist
        """

        self._center_lock.acquire()

        try:
            return self._usb_memory_whitelist

        except:
            raise

        finally:
            self._center_lock.release()

    def get_alert_timestamp(self):
        """
        get alert timestamp
        """

        self._center_lock.acquire()

        try:
            return self._alert_timestamp

        except:
            raise

        finally:
            self._center_lock.release()

    def get_usb_network_port_blacklist(self):
        """
        get usb network whitelist regex
        """

        usb_port_blacklist = '/usb[0-9]*/'
        try:
            block_usb_regex = re.compile('/usb[0-9]*/')
            whitelist = self._usb_network_whitelist

            ports = None
            for k, v in whitelist.items():
                if k.strip().lower() == 'usbbus':
                    ports = [int(n) for n in v.strip().split(',') if n]
                    break
            if not ports:
                return usb_port_blacklist

            if ports:
                black = ''
                for n in range(10):
                    if n in ports:
                        continue
                    black += str(n)
                if not black:
                    return None
                return '/usb[{}]*/'.format(black)
        except:
            self.logger.error(grac_format_exc())
            return usb_port_blacklist


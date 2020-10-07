#! /usr/bin/env python3

#-----------------------------------------------------------------------
import configparser
import threading
import pyudev
import re

from grac_util import GracConfig,GracLog,grac_format_exc,search_file_reversely
from grac_define import *

#-----------------------------------------------------------------------
class GracUdevDispatcher():
    """
    UDEV DISPATCHER
    """

    def __init__(self, data_center):

        threading.Thread.__init__(self)

        self.conf = GracConfig.get_config()
        self.logger = GracLog.get_logger()

        #DATA CENTER
        self.data_center = data_center

        #UDEV
        self.udev_context = pyudev.Context()
        self.udev_monitor = pyudev.Monitor.from_netlink(self.udev_context)
        self.udev_observer = None

    def start_monitor(self):
        """
        start monitor
        """

        self.register_udev_callback()
        self.logger.info('START MONITOR')

    def stop_monitor(self):
        """
        stop monitor
        """

        self.unregister_udev_callback()
        self.logger.info('STOP MONITOR')

    def register_udev_callback(self):
        """
        register callback to async monitor
        """

        if self.udev_observer:
            self.logger.error('!! observer is already running')
            return

        #load filter

        #observer
        self.udev_observer = pyudev.MonitorObserver(self.udev_monitor, self.udev_callback)
        self.udev_observer.start()

        self.logger.info('registered callback')
        
    def unregister_udev_callback(self):
        """
        unregister callback from async monitor
        """

        if self.udev_observer:
            self.udev_observer.stop()
            self.udev_observer = None

        self.logger.info('unregistered callback')

    def udev_callback(self, action, device):
        """
        callback
        """

        self.logger.debug(
            'UEVENT device_node={} device_path={}'.format(
                                                        device.device_node, 
                                                        device.device_path))

        rules_map = self.data_center.get_rules_map()
        module_argv = None

        for target, target_info in rules_map.items():
            for mode, mode_info in target_info.items():
                filter_list = mode_info[RULES_MAP_FILTER]

                for item_list in filter_list:
                    for item in item_list:
                        tp, lhs, rhs, op = item
                        regex = re.compile(rhs)

                        if tp == RULES_MAP_TYPE_ENV:
                            v = device.get(lhs)
                            self.logger.debug(
                                '(env) lhs={} rhs={} op={} v={}'.format(lhs, rhs, op, v))
                            if not self.match_item(regex, op, v):
                                break

                        elif tp == RULES_MAP_TYPE_ATTRS:
                            v = self.get_attrs_value(device.device_path, lhs)
                            self.logger.debug(
                                '(attr) lhs={} rhs={} op={} v={}'.format(lhs, rhs, op, v))
                            if not self.match_item(regex, op, v):
                                break

                        elif tp == RULES_MAP_TYPE_ATTR:
                            v = device.attributes.get(lhs)
                            self.logger.debug(
                                '(attr) lhs={} rhs={} op={} v={}'.format(lhs, rhs, op, v))
                            if not self.match_item(regex, op, v):
                                break

                        elif tp == RULES_MAP_TYPE_PROP:
                            lhs = lhs.lower()
                            if lhs == 'devnode':
                                v = getattr(device, 'device_node')
                                if v:
                                    v = v.split('/')[-1]
                            elif lhs == 'devnodes':
                                v = getattr(device, 'device_node')
                            else:
                                try:
                                    v = getattr(device, lhs)
                                except:
                                    v = None
                            self.logger.debug(
                                '(prop) lhs={} rhs={} op={} v={}'.format(lhs, rhs, op, v))
                            if not self.match_item(regex, op, v):
                                break

                        elif tp == RULES_MAP_TYPE_MODULE:
                            self.logger.debug(
                                '(module) lhs={} rhs={} op={} v={}'.format(lhs, rhs, op, v))
                            module_argv = self.make_module_argv(device, rhs)

                        else:
                            self.logger.error('{} unknown type={}'.format(lhs, tp))
                            break
                    else:
                        ###############################################################
                        # DO TASK!
                        #
                        self.logger.info('RUN MODULE={}'.format(module_argv))
                        module_name = self.data_center.get_module(module_argv[0])
                        do_task = getattr(module_name, 'do_task')
                        do_task(module_argv[1:], self.data_center)
                        return
                        #
                        ###############################################################


    def get_attrs_value(self, path, lhs):
        """
        get value of attributes
        """

        fn = search_file_reversely('/sys/'+path, lhs, 3)
        if not fn:
            return None

        with open(fn) as f:
            return f.read()

    def make_module_argv(self, device, rhs):
        """
        MODULE=xxx 의 xxx값을 모듈명과 인자들로 분리하고 
        ENV{}, ATTR{}, property 인자들을 device의 속성값으로 치환
        """

        res_argv = []
        argv = rhs.split()

        for arg in argv:
            arg = arg.strip()
            if arg[0] == '$':
                if arg[1] == '$':
                    arg = arg[2:]
                else:
                    arg = arg[1:].lower()

            self.logger.debug('--module arg={}'.format(arg))
            if arg.startswith(RULES_MAP_TYPE_ENV.lower()):
                res_argv.append(device.get(arg[4:-1].upper()))

            elif arg.startswith(RULES_MAP_TYPE_ATTRS.lower()):
                res_argv.append(self.get_attrs_value(device.device_path, arg[6:-1]))

            elif arg.startswith(RULES_MAP_TYPE_ATTR.lower()):
                res_argv.append(device.attributes.get(arg[5:-1]))

            elif arg == 'devpath':
                res_argv.append(device.device_path)

            elif arg == 'devnode':
                res_argv.append(device.device_node.split('/')[-1])

            else:
                res_argv.append(arg)

        return res_argv

    def match_item(self, regex, op, v):
        #hmm
        if v == None:
            v = 'None'

        if isinstance(v, bytes):
            v = v.decode('utf8')

        res = False
        if op == '==':
            if regex.search(v):
                res = True
        elif op == '!=': 
            if not regex.search(v):
                res = True
        else:
            self.logger.error('invalid operator={}'.format(op))

        if res:
            self.logger.debug('MATCH!')

        return res

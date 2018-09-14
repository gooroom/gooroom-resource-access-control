#! /usr/bin/env python3

#-----------------------------------------------------------------------
import simplejson as json
import subprocess
import dbus.service
import dbus
import time
import sys
import os

from gi.repository import GLib
from dbus.mainloop.glib import DBusGMainLoop

from grac_util import GracLog,GracConfig,grac_format_exc
from grac_udev_dispatcher import GracUdevDispatcher
from grac_synchronizer import GracSynchronizer
from grac_data_center import GracDataCenter
from grac_printer import GracPrinter
from grac_network import GracNetwork
from grac_define import *

#-----------------------------------------------------------------------
#for decorator parameter
DBUS_NAME = GracConfig.get_config().get('MAIN', 'DBUS_NAME')
DBUS_OBJ = GracConfig.get_config().get('MAIN', 'DBUS_OBJ')
DBUS_IFACE = GracConfig.get_config().get('MAIN', 'DBUS_IFACE')

#-----------------------------------------------------------------------
class Grac(dbus.service.Object):
    """
    |GRAC|
    """

    def __init__(self):

        self.logger = GracLog.get_logger()

        #DBUS
        DBusGMainLoop(set_as_default=True)
        self._loop = None
        self._loop = GLib.MainLoop()

        busname = dbus.service.BusName(DBUS_NAME, bus=dbus.SystemBus())
        dbus.service.Object.__init__(self, busname, DBUS_OBJ)

        #UDEV DISPATCHER
        self.udev_dispatcher = None

        #DATA CENTER
        self.data_center = None

        #GRAC PRINTER
        self.grac_printer = None

        #GRAC NETWORK
        self.grac_network = None

        #MODPROBE usb-storage.ko
        #should change method...
        subprocess.call(['/sbin/modprobe','usb-storage'])

        self.logger.info('GRAC CREATED')

    def __del__(self):
        self.logger.debug('GRAC DESTROYED')

    def run(self):
        """
        GRAC's main loop
        """

        self.logger.info('GRAC RUNNING')

        #DATA CENTER
        self.data_center = GracDataCenter()

        #UDEV DISPATCHER
        self.udev_dispatcher = GracUdevDispatcher(self.data_center)

        #GRAC PRINTER
        self.grac_printer = GracPrinter(self.data_center)

        #NETWORK
        self.grac_network = GracNetwork(self.data_center)

        #RELOAD
        self.reload('')

        self._loop.run()

        self.logger.info('GRAC QUIT')

    @dbus.service.method(DBUS_IFACE)
    def stop(self, args):
        """
        dbus stop
        """

        try:
            self.logger.info('GRAC STOPPING BY DBUS')

            if self.udev_dispatcher:
                self.udev_dispatcher.stop_monitor()

            if self.grac_printer:
                self.grac_printer.teardown()

            if self._loop and self._loop.is_running():
                self._loop.quit()

            self.logger.info('GRAC STOPPED BY DBUS')

        except:
            e = grac_format_exc()
            self.logger.error(e)
            return e

        return GRAC_OK

    @classmethod
    def stop_myself(cls, target):
        """
        stop myself
        """

        system_bus = dbus.SystemBus()
        bus_object = system_bus.get_object(DBUS_NAME, DBUS_OBJ)
        bus_interface = dbus.Interface(bus_object, dbus_interface=DBUS_IFACE)
        return eval('bus_interface.stop(target)')
        
    @dbus.service.method(DBUS_IFACE)
    def reload(self, args):
        """
        reload
        """

        try:
            self.logger.info('GRAC RELOADING BY DBUS')

            self.data_center.show()

            if self.udev_dispatcher:
                self.udev_dispatcher.stop_monitor()
                self.synchronize_json_rule_and_device()
                self.udev_dispatcher.start_monitor()

            if self.grac_printer:
                self.grac_printer.reload()

            if self.grac_network:
                self.grac_network.reload()

            self.logger.info('GRAC RELOADED BY DBUS')
        except:
            e = grac_format_exc()
            self.logger.error(e)
            return e

        return GRAC_OK

    @classmethod
    def reload_myself(cls, target):
        """
        stop myself
        """

        system_bus = dbus.SystemBus()
        bus_object = system_bus.get_object(DBUS_NAME, DBUS_OBJ)
        bus_interface = dbus.Interface(bus_object, dbus_interface=DBUS_IFACE)
        return eval('bus_interface.reload(target)')
        
    def rescan_pci(self):
        """
        rescan pci devices
        """

        try:
            if os.path.exists(META_FILE_PCI_RESCAN):
                with open('/sys/bus/pci/rescan', 'w') as f:
                    f.write('1')
                os.unlink(META_FILE_PCI_RESCAN)
                time.sleep(1) #hmm
                self.logger.info('---rescan pci---')
        except:
            self.logger.error(grac_format_exc())

    def synchronize_json_rule_and_device(self):
        """
        json rule의 모드와 장치의 모드가 다를 경우 동기화
        """

        self.rescan_pci()

        json_rules = self.data_center.get_json_rules()

        for k, v in json_rules.items():
            try:
                media_name = k
                media_type = None

                for sm in GracSynchronizer.supported_media:
                    if media_name == sm[0]:
                        media_type = sm[1]
                        break
                else:
                    continue

                state = v
                if isinstance(state, dict):
                    state = state[JSON_RULE_STATE]

                if state == JSON_RULE_ALLOW:
                    if media_type == MC_TYPE_BCONFIG:
                        GracSynchronizer.recover_bConfigurationValue(media_name)
                    continue

                self.logger.debug('evaluating sync_{}()'.format(media_name))
                eval('GracSynchronizer.sync_{}(state, self.data_center)'.format(media_name))
            except:
                self.logger.error(grac_format_exc())

#-----------------------------------------------------------------------
if __name__ == '__main__':
    """
    main
    """

    #stop for systemd
    if len(sys.argv) > 1 and sys.argv[1] == 'stop':
        Grac.stop_myself('')
        sys.exit(0)

    #reload for systemd
    if len(sys.argv) > 1 and sys.argv[1] == 'reload':
        Grac.reload_myself('')
        sys.exit(0)

    me = None
    try:
        me = Grac()
        me.run()

    except:
        GracLog.get_logger().error('(main) %s' % grac_format_exc())

        if me:
            me.stop('')
        raise

#! /usr/bin/env python3

#-----------------------------------------------------------------------
import simplejson as json
import subprocess
import pyinotify
import dbus.service
import filecmp
import shutil
import dbus
import time
import sys
import os

from grac_define import EXTENSION_FULLPATH
from grac_util import catch_user_id
from grac_api import GracApi

#-----------------------------------------------------------------------
#for clipboard...
'''
g_session_user, g_session_display = catch_user_id()
if len(g_session_user) > 0 and g_session_user[0] == '+':
    g_session_user = g_session_user[1:]
os.environ['DISPLAY'] = g_session_display
os.environ['XAUTHORITY'] = '/home/{}/.Xauthority'.format(g_session_user)
'''

#-----------------------------------------------------------------------
import gi
gi.require_version('Gtk', '3.0')

from gi.repository import GLib,Gtk,Gdk
from dbus.mainloop.glib import DBusGMainLoop

from grac_util import GracLog,GracConfig,grac_format_exc,red_alert2
from grac_util import make_media_msg
from grac_udev_dispatcher import GracUdevDispatcher
from grac_synchronizer import GracSynchronizer
from grac_data_center import GracDataCenter
from grac_printer import GracPrinter
from grac_network import GracNetwork
from grac_inotify import *
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

        #DBUS FOR SIGNAL(SCREENCAPTURE/CLIPBOARD)
        self.signal_dbus = dbus.SystemBus()
        self.signal_dbus.add_signal_receiver(
            self.catch_grac_noti_forward, 
            dbus_interface='kr.gooroom.GRACDEVD', 
            signal_name='grac_noti_forward')

        self.logger.info('GRAC CREATED')

    def catch_grac_noti_forward(self, param):
        """
        catch grac_noti_forward(screen-capture/clipboard)
        """

        self.grac_noti(param)

    def __del__(self):
        self.logger.debug('GRAC DESTROYED')

    def run(self):
        """
        GRAC's main loop
        """

        self.logger.info('GRAC RUNNING')

        #DATA CENTER
        self.data_center = GracDataCenter(self)

        #UDEV DISPATCHER
        self.udev_dispatcher = GracUdevDispatcher(self.data_center)

        #GRAC PRINTER
        self.grac_printer = GracPrinter(self.data_center)

        #NETWORK
        self.grac_network = GracNetwork(self.data_center)

        #SOUND/MICROPHONE INOTIFY THREAD
        self.data_center.sound_mic_inotify = None

        #CLIPBOARD
        self.clipboard_init = False

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
        
    def reload_snd_mic_dev(self, wm):
        """
        reload sound|microphone devices
        """

        dev_snd_path = "/dev/snd/"
        file_list = os.listdir(dev_snd_path)

        for control_file in file_list:
            if control_file.find("control") is not -1:
                idx = control_file.split("controlC")
                wm.add_watch(
                    dev_snd_path + control_file, 
                    pyinotify.IN_ACCESS, 
                    rec=True)

    def check_cert(self):
        """
        check server certification
        """

        if not os.path.exists(META_PREV_SERVER_CERT_PATH):
            shutil.copyfile(META_SERVER_CERT_PATH, META_PREV_SERVER_CERT_PATH)
        else:
            if not filecmp.cmp(META_PREV_SERVER_CERT_PATH, META_SERVER_CERT_PATH):
                user_json_rules_path = GracConfig.get_config().get(
                                                    'MAIN', 
                                                    'GRAC_USER_JSON_RULES_PATH')
                if os.path.exists(user_json_rules_path):
                    os.remove(user_json_rules_path)
                if os.path.exists(META_PREV_SERVER_CERT_PATH):
                    os.remove(META_PREV_SERVER_CERT_PATH)

    @dbus.service.method(DBUS_IFACE)
    def reload(self, args):
        """
        reload
        """

        try:
            self.logger.info('GRAC RELOADING BY DBUS')

            try:
                self.check_cert()
            except:
                self.logger.error(grac_format_exc())

            self.data_center.show()

            #SOUND/MICROPHONE INOTIFY THREAD
            try:
                if not self.data_center.sound_mic_inotify: 
                    self.data_center.sound_mic_inotify = \
                        SoundMicInotify(self.data_center)
                    self.data_center.sound_mic_inotify.WM = \
                    self.start_sound_mic_inotify(self.data_center)
                else:
                    self.reload_snd_mic_dev(
                        self.data_center.sound_mic_inotify.WM)
            except:
                self.logger.error(grac_format_exc())

            #CLIPBOARD
            '''
            try:
                if not self.clipboard_init:
                    self.clipboard_init = True
                    self.start_clipboard_handler(self.data_center)
            except:
                self.logger.error(grac_format_exc())
            '''

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
            self.logger.error(grac_format_exc())

        return GRAC_OK

    @dbus.service.method(DBUS_IFACE, sender_keyword='sender', in_signature='s', out_signature='s')
    def do_api(self, args, sender=None):
        """
        media api method
        """

        try:
            msg = json.loads(args)
            api = GracApi(self)
            return api.do_api(msg, sender)

        except: 
            e = grac_format_exc()
            self.logger.error(e)
            return args

    @dbus.service.signal(DBUS_IFACE, signature='v')
    def grac_noti(self, msg):
        """
        send signal to user session 
        so as to send grac-noti
        """

        pass

    @dbus.service.signal(DBUS_IFACE, signature='v')
    def grac_letter(self, msg):
        """
        send signal to user session 
        so as to send grac-letter
        """

        pass

    @dbus.service.signal(DBUS_IFACE, signature='s')
    def media_usb_info(self, msg):
        """
        send signal to user session 
        so as to send media-usb-info
        """

        pass

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
                '''
                with open(META_FILE_PCI_RESCAN, 'r') as f:
                    lines = f.read().split('\n')
                    for line in lines:
                        if not line:
                            continue
                        media, path = line.split('=>')
                        parent_path = '/'.join(path.split('/')[:-2])
                        if os.path.exists(parent_path+'/rescan'):
                            with open(parent_path+'/rescan', 'w') as f2:
                                f2.write('1')
                                self.logger.info(
                                    'rescan media={}:parent_path={}'.format(
                                                                        media, 
                                                                        parent_path))
                '''
                with open('/sys/bus/pci/rescan', 'w') as f:
                    f.write('1')
                os.unlink(META_FILE_PCI_RESCAN)
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

                if media_type == MC_TYPE_NOSYNC:
                    continue

                state = v
                if isinstance(state, dict):
                    state = state[JSON_RULE_STATE]

                if media_type != MC_TYPE_ALLOWSYNC and state == JSON_RULE_ALLOW:
                    if media_type == MC_TYPE_BCONFIG:
                        GracSynchronizer.recover_bConfigurationValue(media_name)
                    continue

                self.logger.debug('evaluating sync_{}()'.format(media_name))
                eval('GracSynchronizer.sync_{}(state, self.data_center)'.format(media_name))
            except:
                self.logger.error(grac_format_exc())

    def start_sound_mic_inotify(self, data_center):
        """
        start inotify thread for sound/microphone
        """

        wm = pyinotify.WatchManager()
        dev_snd_path = "/dev/snd/"
        file_list = os.listdir(dev_snd_path)

        for control_file in file_list:
            if control_file.find("control") is not -1:
                idx = control_file.split("controlC")
                wm.add_watch(
                    dev_snd_path + control_file, 
                    pyinotify.IN_ACCESS, 
                    rec=True)

        notifier = pyinotify.ThreadedNotifier(wm, data_center.sound_mic_inotify)
        notifier.daemon = True
        notifier.start()
        return wm

    def start_clipboard_handler(self, data_center):
        """
        start clipboard handler
        """

        clip = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        if clip:
            clip.connect('owner-change', self.clipboard_handler, data_center)
        else:
            self.logger.error('CLIPBOARD NULL')

    def clipboard_handler(self, *args):
        """
        clipboard handler
        """

        try:
            #CHECK EXTENSION(screencapture/clipboard)
            if os.path.exists(EXTENSION_FULLPATH):
                return

            data_center = args[2]
            clipboard_rule = data_center.get_media_state(JSON_RULE_CLIPBOARD)
            if clipboard_rule == JSON_RULE_ALLOW:
                return

            clip = args[0]
            if clip.wait_is_text_available() and clip.wait_for_text():
                Gtk.Clipboard.set_text(clip, "", 0)
                Gtk.Clipboard.clear(clip)
                GracLog.get_logger().info('clipboard text blocked')
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_CLIPBOARD, clipboard_rule)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                    grmcode, data_center)#, flag=RED_ALERT_ALERTONLY)

            elif clip.wait_is_image_available() and clip.wait_for_image():
                new_pb = gi.repository.GdkPixbuf.Pixbuf()
                Gtk.Clipboard.set_image(clip, new_pb)
                Gtk.Clipboard.clear(clip)
                GracLog.get_logger().info('clipboard image blocked')
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_CLIPBOARD, clipboard_rule)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, 
                    grmcode, data_center)#, flag=RED_ALERT_ALERTONLY)

            elif clip.wait_is_rich_text_available(Gtk.TextBuffer()) \
                and clip.wait_for_rich_text():
                Gtk.Clipboard.clear(clip)
                GracLog.get_logger().info('clipboard rich blocked')

            elif clip.wait_is_uris_available() and clip.wait_for_uris():
                Gtk.Clipboard.clear(clip)
                GracLog.get_logger().info('clipboard uris blocked')
        except:
            GracLog.get_logger().error(grac_format_exc())

#-----------------------------------------------------------------------
if __name__ == '__main__':
    """
    ^^.
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
        GracLog.get_logger().error(grac_format_exc())

        if me:
            me.stop('')
        sys.exit(0)
    sys.exit(0)

#! /usr/bin/env python3

#-----------------------------------------------------------------------
import threading
import shutil
import time
import cups
import glob
import dbus
import os

from grac_util import make_media_msg,red_alert2
from grac_util import GracLog,grac_format_exc
from grac_define import *

#-----------------------------------------------------------------------
class GracPrinter:
    """
    control local/remote printers
    """

    def __init__(self, data_center):
        
        self.logger = GracLog.get_logger()

        #DATA CENTER
        self.data_center = data_center

        #CUPS
        self._conn = cups.Connection()

        #WATCHDOG
        self._watchdog_thread = None
        self._watchdog_dead = False
        self._watchdog_bark_on = False

    def teardown(self):
        """
        tear down
        """

        self._watchdog_dead = True
        if self._watchdog_thread:
            self._watchdog_thread.join()
        self.logger.info('tear down')

    def reload(self):
        """
        grac reload
        """

        self.logger.info('reloading')

        try:
            #printer state is allow or not
            json_rules = self.data_center.get_json_rules()
            v = json_rules[JSON_RULE_PRINTER]
            state = None
            if isinstance(v, str):
                state = v
            else:
                state = v[JSON_RULE_STATE]

            if state == JSON_RULE_ALLOW:
                self.stop_watching()
                self.load_and_add_printers()
                self.start_cups_browsed()
            else:
                self.stop_cups_browsed()
                self.save_printers()
                self.delete_printers()
                self.start_to_watch()

        except:
            self.logger.error(grac_format_exc())

        self.logger.info('reloaded')

    def start_to_watch(self):
        """
        start to watch printers
        """

        self._watchdog_bark_on = True

        if not self._watchdog_thread:
            self._watchdog_thread = threading.Thread(target=self._watch_printers)
            self._watchdog_thread.start()

    def stop_watching(self):
        """
        stop watching printers
        """

        self._watchdog_bark_on = False

    def _watch_printers(self):
        """
        thread target
        """

        while True:
            if self._watchdog_dead:
                break

            if self._watchdog_bark_on:
                try:
                    printers = self._conn.getPrinters()

                    for printer_name in printers:
                        self._conn.deletePrinter(printer_name)
                        self.logger.debug('(watch) delete printer({})'.format(printer_name))
                        logmsg, notimsg, grmcode = \
                            make_media_msg(JSON_RULE_PRINTER, JSON_RULE_DISALLOW)
                        red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, self.data_center)
                except:
                    self.logger.error(grac_format_exc())

            time.sleep(1)

    def load_and_add_printers(self):
        """
        저장되어 있는 프린터정보를 이용해서 프린터 추가
        """

        backup_path = META_FILE_PRINTER_BACKUP_PATH
        if not os.path.exists(backup_path):
            return

        printer_list = backup_path+'/'+META_FILE_PRINTER_LIST
        if not os.path.exists(printer_list):
            return

        with open(printer_list) as f:
            try:
                for print_info in f.readlines():
                    print_name, uri = print_info.split(GRAC_PRINTER_DELIM)
                    ppd = cups.PPD(backup_path+'/'+print_name)

                    self._conn.addPrinter(print_name, device=uri, ppd=ppd)
                    self._conn.enablePrinter(print_name)
                    self._conn.acceptJobs(print_name)
            except:
                self.logger.error(grac_format_exc())
        
        shutil.rmtree(backup_path)

    def save_printers(self):
        """
        사용하고 있는 프린터 정보를 저장.
        """

        try:
            printers = self._conn.getPrinters()
            if len(printers) <= 0:
                return
        except:
            self.logger.error(grac_format_exc())
            return

        backup_path = META_FILE_PRINTER_BACKUP_PATH
        os.makedirs(backup_path, exist_ok=True)

        with open(backup_path+'/'+META_FILE_PRINTER_LIST, 'w') as f:
            for printer_name, v in printers.items():
                try:
                    uri = v['device-uri']
                    ppd = self._conn.getPPD(printer_name)

                    with open(ppd) as f2:
                        with open(backup_path+'/'+printer_name, 'w') as f3:
                            f3.write(f2.read())

                    f.write('{}{}{}\n'.format(printer_name, GRAC_PRINTER_DELIM, uri))
                except:
                    self.logger.error(grac_format_exc())

    def delete_printers(self):
        """
        사용하고 있는 프린터를 삭제
        """

        try:
            printers = self._conn.getPrinters()
            if len(printers) <= 0:
                return
        except:
            self.logger.error(grac_format_exc())
            return

        for printer_name in printers:
            try:
                self._conn.deletePrinter(printer_name)
                self.logger.debug('delete printer({})'.format(printer_name))
                logmsg, notimsg, grmcode = \
                    make_media_msg(JSON_RULE_PRINTER, JSON_RULE_DISALLOW)
                red_alert2(logmsg, notimsg, JLEVEL_DEFAULT_NOTI, grmcode, self.data_center)
            except:
                self.logger.error(grac_format_exc())

    def start_cups_browsed(self):
        """
        start cups-browsed.service
        """

        try:
            bus = dbus.SystemBus()
            systemd1 = bus.get_object(SD_BUS_NAME, SD_BUS_OBJ)
            manager = dbus.Interface(systemd1, dbus_interface=SD_BUS_IFACE)
            manager.StartUnit('cups-browsed.service', "fail")
        except:
            self.logger.error(grac_format_exc())

    def stop_cups_browsed(self):
        """
        stop cups-browsed.service
        """

        try:
            bus = dbus.SystemBus()
            systemd1 = bus.get_object(SD_BUS_NAME, SD_BUS_OBJ)
            manager = dbus.Interface(systemd1, dbus_interface=SD_BUS_IFACE)
            manager.StopUnit('cups-browsed.service', "fail")
        except:
            self.logger.error(grac_format_exc())

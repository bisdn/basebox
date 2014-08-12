#!/usr/bin/python

import os
import logging
import basecore
import subprocess
import ipv6prefix


class RAdvd(object):
    """Class for controlling an radvd instance"""
    STATE_STOPPED = 0
    STATE_ANNOUNCING = 1
    radvd_binary = '/usr/sbin/radvd'
    
    def __init__(self, baseCore, devname, conffiledir='.'):
        self.logger = logging.getLogger('proact')
        self.state = self.STATE_STOPPED
        self.baseCore = baseCore
        self.devname = devname
	self.dnssl = 'proact.'
	self.rdnss = '3000:1::1'
        self.prefixes = []
        self.process = None
        self.conffiledir = conffiledir
        self.conffile = self.conffiledir + '/radvd.' + self.devname + '.conf'
        self.logfile = self.conffiledir + '/radvd.' + self.devname + '.log'
        self.pidfile = self.conffiledir + '/radvd.' + self.devname + '.pid'
    
    def __str__(self, *args, **kwargs):
        s_prefixes = ''
        for prefix in self.prefixes:
            s_prefixes += str(prefix) + ' '
        return '<RAdvd [' + self.devname + '] ' + '[state: ' + str(self.state) + '] ' + '[prefix(es): ' + s_prefixes + ' >'
    
    def addPrefix(self, prefix):
        if not isinstance(prefix, ipv6prefix.IPv6Prefix):
            self.baseCore.logger.warn('not of type IPv6Prefix, ignoring')
            return
        if not prefix in self.prefixes:
            self.prefixes.append(prefix)
        
    def delPrefix(self, prefix):
        if not isinstance(prefix, ipv6prefix.IPv6Prefix):
            self.baseCore.logger.warn('not of type IPv6Prefix, ignoring')
            return
        if prefix in self.prefixes:
            self.prefixes.remove(prefix)

    def start(self):
        if self.process != None:
            self.stop()
        self.__rebuild_config()
        if len(self.prefixes) == 0:
            self.baseCore.logger.warn('no prefixes available for radvd, suppressing start ' + str(self))
            return
        self.state = self.STATE_ANNOUNCING
        radvd_cmd = self.radvd_binary + ' -C ' + self.conffile + ' -p ' + self.pidfile + ' -m ' + 'logfile' + ' -l ' + self.logfile + ' -d ' + '1' 
        self.baseCore.logger.debug('radvd start: executing command => ' + str(radvd_cmd))
        self.process = subprocess.Popen(radvd_cmd.split())
        self.baseCore.addEvent(basecore.BaseCoreEvent(self, basecore.BaseCore.EVENT_RADVD_START))
    
    def stop(self):
        self.state = self.STATE_STOPPED
        if self.process == None:
            return
        kill_cmd = 'kill -KILL ' + str(self.process.pid)
        self.baseCore.logger.debug('radvd stop: executing command => ' + str(kill_cmd))
        subprocess.call(kill_cmd.split())
        self.process = None
        self.baseCore.addEvent(basecore.BaseCoreEvent(self, basecore.BaseCore.EVENT_RADVD_STOP))
        os.unlink(self.conffile)

    def restart(self):
        if self.state == self.STATE_ANNOUNCING:
            self.stop()
        if len(self.prefixes) == 0:
            return
        self.__rebuild_config()
        self.start()

    def __rebuild_config(self):
        try:
            f = open(self.conffile, 'w')
            f.write('interface ' + self.devname + ' {\n');
            f.write('    IgnoreIfMissing on;\n')
            f.write('    AdvSendAdvert on;\n')
            f.write('    MaxRtrAdvInterval 4;\n')
            f.write('    MinRtrAdvInterval 3;\n')
            f.write('    RDNSS ' + str(self.rdnss) + ' {\n')
            f.write('    };\n')
            f.write('    DNSSL ' + str(self.dnssl) + ' {\n')
            f.write('    };\n')

            for prefix in self.prefixes:
                f.write('    prefix ' + str(prefix.prefix) + '/' + str(prefix.prefixlen) + '\n')
                f.write('    {\n')
                f.write('        AdvOnLink on;\n')
                f.write('        AdvAutonomous on;\n')
                f.write('        AdvValidLifetime 3600;\n')
                f.write('        AdvPreferredLifetime 3600;\n')
                f.write('        DeprecatePrefix on;\n')
                f.write('        AdvValidLifetime 120;\n')
                f.write('        AdvPreferredLifetime 120;\n')
                f.write('    };\n')
                f.write('\n')
            f.write('};\n')
            f.close()
        except IOError:
            pass




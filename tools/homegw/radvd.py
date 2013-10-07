#!/usr/bin/python

import subprocess
import homegw

class IPv6Prefix(object):
    """Class for abstracting a single IPv6 prefix"""
    
    def __init__(self, prefix, prefixlen):
        self.prefix = prefix
        self.prefixlen = prefixlen

    def __str__(self, *args, **kwargs):
        return '<IPv6Prefix [' + str(self.prefix) + '' + str(self.prefixlen) + '] ' + object.__str__(self, *args, **kwargs) + ''


class RAdvd(object):
    """Class for controlling an radvd instance"""
    STATE_STOPPED = 0
    STATE_ANNOUNCING = 1
    radvd_binary = '/sbin/radvd'
    
    def __init__(self, parent, devname, conffiledir='.'):
        self.state = self.STATE_STOPPED
        self.parent = parent
        self.devname = devname
        self.prefixes = []
        self.process = None
        self.conffiledir = conffiledir
        self.conffile = self.conffiledir + '/radvd.' + self.devname + '.conf'
        self.pidfile = self.conffiledir + '/radvd.' + self.devname + '.pid'
    
    def __str__(self, *args, **kwargs):
        return '<RAdvd [' + self.devname + '] ' + '[state: ' + str(self.state) + '] ' + object.__str__(self, *args, **kwargs) + ' >'
    
    def add_prefix(self, prefix):
        if not isinstance(prefix, IPv6Prefix):
            print "not of type IPv6Prefix, ignoring"
            return
        self.prefixes.append(prefix)
        
    def del_prefix(self, prefix):
        if not isinstance(prefix, IPv6Prefix):
            print "not of type IPv6Prefix, ignoring"
            return
        if prefix in self.prefixes:
            self.prefixes.remove(prefix)

    def start(self):
        if self.process != None:
            self.stop()
        self.state = self.STATE_ANNOUNCING
        self.__rebuild_config()
        print "starting RAs on " + self.devname
        radvd_cmd = self.radvd_binary + ' -C ' + self.conffile + ' -p ' + self.pidfile
        print radvd_cmd.split() 
        self.process = subprocess.Popen(radvd_cmd.split())
    
    def stop(self):
        self.state = self.STATE_STOPPED
        if self.process == None:
            return
        print "stopping RAs on " + self.devname
        kill_cmd = 'kill -INT ' + str(self.process.pid)
        subprocess.call(kill_cmd.split())
        self.process = None

    def __rebuild_config(self):
        try:
            f = open(self.conffile, 'w')
            f.write('interface ' + self.devname + ' {\n');
            f.write('    AdvSendAdvert on;\n')
            for prefix in self.prefixes:
                f.write('    prefix ' + prefix.prefix + '/' + str(prefix.prefixlen) + '\n')
                f.write('    {\n')
                f.write('        AdvOnLink on;\n')
                f.write('        AdvAutonomous on;\n')
                f.write('    };\n')
                f.write('\n')
            f.write('};\n')
            f.close()
        except IOError:
            pass




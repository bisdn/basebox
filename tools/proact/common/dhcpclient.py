#!/usr/bin/python

import subprocess
import basecore
from ipv6prefix import IPv6Prefix


class DhcpClient(object):
    """Class for controlling an ISC DHCP client in ProAct"""
    dhclient_binary = '/sbin/dhclient'
    STATE_PREFIX_DETACHED = 0
    STATE_PREFIX_ATTACHED = 1
    
    def __init__(self, baseCore, devname, pidfilebase='/var/run'):
        self.state = self.STATE_PREFIX_DETACHED
        self.baseCore = baseCore
        self.devname = devname
        self.pidfilebase = pidfilebase
        self.pidfile = self.pidfilebase + '/dhclient6.' + self.devname + '.pid'
        self.old_prefixes = []
        self.new_prefixes = []
        try:
            with open(self.pidfile):
                self.killClient() 
        except IOError:
            pass


    def __del__(self):
        if self.state == self.STATE_PREFIX_ATTACHED:
            self.sendRelease()
    
    
    def __str__(self, *args, **kwargs):
        s_prefixes = ''
        for prefix in self.new_prefixes:
            s_prefixes += str(prefix) + ' '
        return '<DhcpClient [' + self.devname + '] [state: ' + str(self.state) + '] ' + s_prefixes + ' ' + '>'


    def sendRequest(self):
        'call ISC dhclient with following parameters: /sbin/dhclient -v -6 -P -pf ${PIDFILE} ${IFACE}'
        try:
            dhclient_cmd = self.dhclient_binary + ' -6 -q -P -1 -pf ' + self.pidfile + ' ' + self.devname
            print 'DHCP request: executing command => ' + str(dhclient_cmd.split())
            self.process = subprocess.Popen(dhclient_cmd.split(), shell=False, stdout=subprocess.PIPE)
            resultstring = self.process.communicate()
            
            self.old_prefixes = []
            self.new_prefixes = []
          
            try:
                for line in resultstring[0].split('\n'):
                    elems = {}
                    for token in line.split():
                            (key, value) = token.split('=')
                            elems[key] = value
                            print str(key) + '=' + str(elems[key])
    
                    self.reason = elems.get('reason', None)
    
                    if self.reason == 'PREINIT6':
                        pass
                    elif self.reason == 'BOUND6':
                        self.bind(elems)
                    elif self.reason == 'RENEW6':
                        self.bind(elems)
                    elif self.reason == 'REBIND6':
                        self.bind(elems)
                    elif self.reason == 'REBOOT':
                        self.bind(elems)
                    elif self.reason == 'EXPIRE':
                        self.expire(elems)
                    elif self.reason == 'FAIL':
                        self.fail(elems)
                    elif self.reason == 'STOP':
                        self.stop(elems)
                    elif self.reason == 'RELEASE':
                        self.unbind(elems)
                    elif self.reason == 'NBI':
                        pass
                    elif self.reason == 'TIMEOUT':
                        self.timeout(elems)
                    else:
                        print 'unknown reason ' + str(self.reason) + ' detected from dhclient'
                        return
            except ValueError, e:
                print 'ignoring line ' + str(line)
        except:
            print 'error executing dhclient'
        
    
    def sendRelease(self):
        if self.state == self.STATE_PREFIX_DETACHED:
            return
        print "sending DHCP release: " + str(self)
        try:
            dhclient_cmd = self.dhclient_binary + ' -6 -q -N -r -pf ' + self.pidfile + ' ' + self.devname
            print 'DHCP release: executing command => ' + str(dhclient_cmd.split())
            process = subprocess.Popen(dhclient_cmd.split(), shell=False, stdout=subprocess.PIPE)
            rc = process.communicate()
            if process.returncode == 0:
                self.state = self.STATE_PREFIX_DETACHED
                self.baseCore.addEvent(basecore.BaseCoreEvent(self, basecore.BaseCore.EVENT_PREFIX_DETACHED))
        except:
            print 'unable to release prefixes ' + str(self)
        

    def killClient(self):
        """destroy any running dhclient process associated with this object"""
        try:
            with open(self.pidfile) as f:
                kill_cmd = 'kill -INT ' + f.readline()
                #print "kill-cmd: " + kill_cmd
                subprocess.call(kill_cmd.split())
                #if os.path.exists(self.pidfile):
                #    os.unlink(self.pidfile)
                return True
        except IOError:
            return False
        
    def is_PREFIX_attached(self):
        if self.state == self.STATE_PREFIX_ATTACHED:
            return True
        else:
            return False

    def bind(self, elems):
        try:
            print 'binding DHCP reply: ' + str(self)
            
            reason = elems.get('reason', None)
            s_new_prefix = elems.get('new_ip6_prefix', None)
            s_old_prefix = elems.get('old_ip6_prefix', None)
            old_prefix = IPv6Prefix(s_old_prefix.split('/')[0], s_old_prefix.split('/')[1])
            new_prefix = IPv6Prefix(s_new_prefix.split('/')[0], s_new_prefix.split('/')[1])
            if not old_prefix in self.old_prefixes:
                self.old_prefixes.append(old_prefix)
            if not new_prefix in self.new_prefixes:
                self.new_prefixes.append(new_prefix)
            
            self.state = self.STATE_PREFIX_ATTACHED
            self.baseCore.addEvent(basecore.BaseCoreEvent(self, basecore.BaseCore.EVENT_PREFIX_ATTACHED))
        except IndexError:
            pass
        except ValueError, e:
            print 'unable to acquire prefixes ' + str(self)

    def expire(self, elems):
        print 'DHCP session expired (renewal failed): ' + str(self)
        self.unbind(elems)
    
    def fail(self, elems):
        print 'DHCP request failed (no servers found): ' + str(self)
        self.unbind(elems)

    def stop(self, elems):
        print 'DHCP session stop (dhclient graceful shutdown): ' + str(self)
    
    def unbind(self, elems):
        self.state = self.STATE_PREFIX_DETACHED
        self.baseCore.addEvent(basecore.BaseCoreEvent(self, basecore.BaseCore.EVENT_PREFIX_DETACHED))
    
    def timeout(self, elems):
        print 'DHCP request failed (no servers found, but old lease available): ' + str(self)
        self.bind(elems)


if __name__ == "__main__":
    pass

    

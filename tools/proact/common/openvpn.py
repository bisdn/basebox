#!/usr/bin/python

import os
import signal
import basecore
import subprocess

class OpenVpnException(Exception):
    """base exception for all openvpn related errors"""
    def __init__(self, serror):
        self.serror = serror
    def __str__(self):
        return '<OpenVpnException: ' + str(self.serror) + '>'
    
    
class OpenVpn(object):
    """openvpn abstraction"""
    openvpn_binary = '/usr/sbin/openvpn'
    kill_binary = '/usr/bin/kill'
    def __init__(self, owner, **kwargs):
        self.owner = owner
        self.keywords = ['conffile', 'mode', 'local', 'remote', 'port', 'proto', 'dev', 'devtype', 'up', 'down', 'verb', 'complzo', 'persisttun', 'ca', 'cert', 'key', 'dh', 'tlsclient', 'tlsserver', 'keepalive', 'logappend', 'pull', 'writepid' ]
        for keyword in self.keywords:
            if keyword in kwargs:
                setattr(self, keyword, kwargs[keyword])
        self.conffile = kwargs.get('conffile', './openvpn.conf')
        self.writepid = kwargs.get('writepid', './openvpn.pid')
        self.process = None
        self.__build_config()
        
    def __str__(self):
        s_params = ''
        for param in dir(self):
            if param in self.keywords:
                s_params += param + ':' + str(getattr(self, param)) + ' '
        return '<OpenVpn: ' + s_params + '>'
        
    def __build_config(self):    
        with open(self.conffile, 'w') as f:
            if hasattr(self, 'mode'):
                f.write('mode ' + self.mode + '\n')
            if hasattr(self, 'local'):
                f.write('local ' + self.local + '\n')
            if hasattr(self, 'remote'):
                f.write('remote ' + self.remote + '\n')
            if hasattr(self, 'port'):
                f.write('port ' + str(self.port) + '\n')
            if hasattr(self, 'proto'):
                f.write('proto '+ self.proto + '\n')
            if hasattr(self, 'dev'):
                f.write('dev ' + self.dev + '\n')
            if hasattr(self, 'devtype'):
                f.write('dev-type ' + self.devtype + '\n')
            if hasattr(self, 'verb'):
                f.write('verb ' + str(self.verb))
            if hasattr(self, 'complzo'):
                f.write('comp-lzo ' + self.complzo + '\n')
            if hasattr(self, 'persisttun'):
                f.write('persist-tun' + '\n')
            if hasattr(self, 'keepalive'):
                f.write('keepalive ' + str(self.keepalive) + '\n')
            if hasattr(self, 'tlsclient'):
                f.write('tls-client' + '\n')
            if hasattr(self, 'tlsserver'):
                f.write('tls-server' + '\n')
            if hasattr(self, 'ca'):
                f.write('ca ' + self.ca + '\n')
            if hasattr(self, 'cert'):
                f.write('cert ' + self.cert + '\n')
            if hasattr(self, 'key'):
                f.write('key ' + self.key + '\n')
            if hasattr(self, 'dh'):
                f.write('dh ' + self.dh + '\n')
            if hasattr(self, 'logappend'):
                f.write('log-append ' + self.logappend + '\n')
            if hasattr(self, 'up'):
                f.write('up ' + self.up + '\n')
            if hasattr(self, 'down'):
                f.write('down ' + self.down + '\n')
            if hasattr(self, 'pull'):
                f.write('pull' + '\n')
            f.write('writepid ', self.writepid)
            
                
    def start(self):
        try:
            if self.process != None:
                self.stop()
            self.__build_config()
            start_cmd = self.openvpn_binary + ' --config ' + self.conffile
            print 'start OpenVpn: executing command => ' + start_cmd
            self.process = subprocess.Popen(start_cmd.split(), shell=False, stdout=subprocess.PIPE)
        except:
            print 'starting OpenVpn via ' + start_cmd + ' failed'
        
    def stop(self):
        try:
            if self.process == None:
                return
            with self.writepid as f:
                os.kill(f.read(), signal.SIGINT)
                self.process = None
        except:
            print 'stopping OpenVpn process failed'
        
        
if __name__ == "__main__":
    openvpn = OpenVpn(None, mode='server', complzo='yes', local='1.1.1.1', port=1194, proto='tcp-server', persisttun='yes')
    print openvpn
     
    
    
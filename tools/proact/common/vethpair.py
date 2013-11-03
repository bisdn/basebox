#!/usr/bin/python

import subprocess

class VethPair(object):
    """
    """
    def __init__(self, devname1, devname2):
        self.devname1 = devname1
        self.devname2 = devname2
        self.create()
    def __str__(self):
        return '<VethPair (' + self.devname1 + ',' + self.devname2 + ')>'
    def create(self): 
        """
        creates a single veth pair
        """
        scmd = '/sbin/ip link add ' + \
                'name ' + self.devname1 + ' ' + \
                'type veth peer ' + \
                'name ' + self.devname2
        print 'calling ' + scmd 
        subprocess.call(scmd.split())
        for devname in ( self.devname1, self.devname2 ):
            scmd = '/sbin/ip link set up dev ' + devname
            print 'calling ' + scmd 
            subprocess.call(scmd.split())
    def destroy(self):
        for devname in ( self.devname1, self.devname2 ):
            scmd = '/sbin/ip link set down dev ' + devname
            print 'calling ' + scmd 
            subprocess.call(scmd.split())
        scmd = '/sbin/ip link del dev ' + self.devname1
        print 'calling ' + scmd
        subprocess.call(scmd.split())

#!/usr/bin/python


import sys
import os
import time
import atexit
import signal


class Daemon(object):
    """base support for daemonizing a python script"""
    def __init__(self, **kwargs):
        """
        parameters in kwargs:
        - pidfile
        - stdin
        - stdout
        - stderr
        """
        self.pidfile = kwargs.get('pidfile', './daemon.pid')
        self.stdin   = kwargs.get('stdin', '/dev/null')
        self.stdout  = kwargs.get('stdout', '/dev/null')
        self.stderr  = kwargs.get('stderr', '/dev/null')
        
    def daemonize(self):
        """
        
        """
        # close all file descriptors except stdin, stdout, stderr
        for (dirname, dirnames, filedescriptors) in os.walk('/proc/self/fd'):
            for fd in filedescriptors:
                try:
                    if int(fd) in (sys.stdin.fileno(), sys.stdout.fileno(), sys.stderr.fileno()):
                        continue
                    print sys.stdin
                    os.close(int(fd))
                except OSError, e:
                    sys.stderr.write('closing fd %s failed: %d (%s)\n' % (fd, e.errno, e.strerror))
                    pass
        
        # set signal handlers back to default
        for i in range(1,32):
            try:
                if i in (signal.SIGSTOP, signal.SIGKILL):
                    continue
                signal.signal(i, signal.SIG_DFL)
            except RuntimeError, e:
                print i
                print e
            
        # reset sigprocmask
        # TODO ...
        
        # sanitize environment
        # TODO ...
        
        # call fork (1)
        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0) # terminate the initial parent process
        except OSError, e:
            sys.stderr.write('first syscall to fork() failed: %d (%s)\n' % (e.errno, e.strerror))
            sys.exit(1)
            
        # call setsid()
        os.chdir("/")
        os.setsid()
        os.umask(0) 

        # call fork (2)
        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0) # terminate the initial parent process
        except OSError, e:
            sys.stderr.write('first syscall to fork() failed: %d (%s)\n' % (e.errno, e.strerror))
            sys.exit(1)
                     
        print 'blub1'
           
        # redirect standard file descriptors
        sys.stdout.flush()
        sys.stderr.flush()
        si = file(self.stdin, 'r')
        so = file(self.stdout, 'a+')
        se = file(self.stderr, 'a+', 0)
        os.dup2(si.fileno(), sys.stdin.fileno())
        os.dup2(so.fileno(), sys.stdout.fileno())
        os.dup2(se.fileno(), sys.stderr.fileno())
            
        atexit.register(self.delpid)
        pid = str(os.getpid())
        file(self.pidfile,'w+').write("%s\n" % pid)
        
        
        
        def delpid(self):
            os.remove(self.pidfile)



if __name__ == "__main__":
    Daemon(pidfile='./test.pid').daemonize() 
    
    
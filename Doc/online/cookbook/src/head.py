#!/usr/local/bin/python
# head.py
import os, sys, signal

ECF_PORT = ^ECF_PORT:0^
XSVR = "ecflow_client --port=%s --host=^ECF_NODE:0^ --" % ECF_PORT
pid = os.getpid()

def xinit(): 
    os.system(XSVR + "init %s"%pid)
def xabort(): 
    os.system(XSVR + "abort %s"%pid)
def xcomplete(): 
    os.system(XSVR + "complete")  
def SigHandler(signum, frame): 
    xabort(); sys.exit(0)

signal.signal (signal.SIGINT,  SigHandler); # ...
os.environ['ECF_PORT'] = "^ECF_PORT:0^"
os.environ['ECF_NAME'] = "^ECF_NAME:0^"
os.environ['ECF_NODE'] = "^ECF_NODE:0^"
os.environ['ECF_PASS'] = "^ECF_PASS:0^"

def xmeter(name, step):
   os.system(XSVR + "meter %s %s"%(name,step))  
def xevent(name):
   os.system(XSVR + "event %s"%name)  
def xlabel(name, info):
   os.system(XSVR + "label %s %s"%(name,info))

signal.signal (signal.SIGHUP,  SigHandler)
signal.signal (signal.SIGQUIT, SigHandler)
signal.signal (signal.SIGILL,  SigHandler)
signal.signal (signal.SIGTRAP, SigHandler)
signal.signal (signal.SIGIOT,  SigHandler)
signal.signal (signal.SIGBUS,  SigHandler)
signal.signal (signal.SIGFPE,  SigHandler)
signal.signal (signal.SIGUSR1, SigHandler)
signal.signal (signal.SIGUSR2, SigHandler)
signal.signal (signal.SIGPIPE, SigHandler)
signal.signal (signal.SIGTERM, SigHandler)
signal.signal (signal.SIGXCPU, SigHandler)
signal.signal (signal.SIGPWR,  SigHandler)

print 'start'
xinit()

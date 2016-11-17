#!/usr/local/bin/python
import os, sys, signal
ECF_PORT = ^ECF_PORT:0^
XECF = "ecflow_client "
XECF += "--port %s --host ^ECF_HOST:0^ "%ECF_PORT
XSVR="sms"
if ECF_PORT > 0: XSVR = XECF + "--"
pid = os.getpid()
def call_init(): os.system(XSVR + "init %s"%pid)
def call_abort(): os.system(XSVR + "abort %s"%pid)
def call_complete(): os.system(XSVR + "complete")  
def SigHandler(signum, frame): 
    call_abort(); sys.exit(0)
signal.signal (signal.SIGINT,  SigHandler); # ...
if (ECF_PORT > 0):
  os.environ['ECF_PORT'] = "^ECF_PORT:0^"
  os.environ['ECF_NAME'] = "^ECF_NAME:0^"
  os.environ['ECF_HOST'] = "^ECF_HOST:0^"
  os.environ['ECF_PASS'] = "^ECF_PASS:0^"
else:
  os.environ['SMS_PROG'] = "^SMS_PROG:0^"
  os.environ['SMSNAME']  = "^SMSNAME:0^"
  os.environ['SMSNODE']  = "^SMSNODE:0^"
  os.environ['SMSPASS']  = "^SMSPASS:0^"

def call_meter(name, step):
   os.system(XSVR + "meter %s %s"%(name,step))  
def call_event(name):
   os.system(XSVR + "event %s"%name)  
def call_label(name, info):
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
call_init()

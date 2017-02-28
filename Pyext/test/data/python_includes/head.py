import os, time, signal
import ecflow

print("cwd: " + os.getcwd())
print("PYTHONPATH=====================================================")
try:
    print(os.environ['PYTHONPATH'].split(os.pathsep))
except KeyError:
    print("Could not get PYTHONPATH")

print("LD_LIBRARY_PATH=====================================================")
try:
    print(os.environ['LD_LIBRARY_PATH'].split(os.pathsep))
except KeyError:
    print("Could not get LD_LIBRARY_PATH")
    
    
class Client(object):
    """Encapsulate communication with the ecflow server. This will automatically call
       the child command init()/complete(), for job start/finish. It will also
       handle exceptions and signals, by calling the abort child command.
       *ONLY* one instance of this class, should be used. Otherwise zombies will be created.
    """
    def __init__(self):
        print ("Creating Client")
        self.ci = ecflow.Client()
        self.ci.set_host_port("%ECF_NODE%","%ECF_PORT%")
        self.ci.set_child_pid(os.getpid())
        self.ci.set_child_path("%ECF_NAME%")
        self.ci.set_child_password("%ECF_PASS%")
        self.ci.set_child_try_no(%ECF_TRYNO%)
    
        print("Only wait 20 seconds, if the server cannot be contacted (note default is 24 hours) before failing")
        self.ci.set_child_timeout(20)
     
        # Abort the task for the following signals
        signal.signal(signal.SIGINT,  self.signal_handler)
        signal.signal(signal.SIGHUP,  self.signal_handler)
        signal.signal(signal.SIGQUIT, self.signal_handler)
        signal.signal(signal.SIGILL,  self.signal_handler)
        signal.signal(signal.SIGTRAP, self.signal_handler)
        signal.signal(signal.SIGIOT,  self.signal_handler)
        signal.signal(signal.SIGBUS,  self.signal_handler)
        signal.signal(signal.SIGFPE,  self.signal_handler)
        signal.signal(signal.SIGUSR1, self.signal_handler)
        signal.signal(signal.SIGUSR2, self.signal_handler)
        signal.signal(signal.SIGPIPE, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
        signal.signal(signal.SIGXCPU, self.signal_handler)
        signal.signal(signal.SIGPWR,  self.signal_handler)
     
    def signal_handler(self,signum, frame):
        print('Aborting: Signal handler called with signal ', signum)
        self.ci.child_abort("Signal handler called with signal " + str(signum));
     
    def __enter__(self):
        print('Calling init')
        self.ci.child_init()
        return self.ci
     
    def __exit__(self,ex_type,value,tb):
        print ("Client:__exit__: ex_type:" + str(ex_type) + " value:" + str(value) + "\n" + str(tb))
        if ex_type != None:
            print('Calling abort')
            self.ci.child_abort("Aborted with exception type " + str(ex_type) + ":" + str(value))
            return False
        print('Calling complete')
        self.ci.child_complete()
        return False 
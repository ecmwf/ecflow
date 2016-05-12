#!/usr/bin/env python
import os
import sys
import time
from errno import ENOENT
from stat import S_IFDIR, S_IFREG, S_IFBLK
from sys import argv, exit
from fuse import FUSE, Operations, LoggingMixIn, FuseOSError, fuse_get_context

try: import ecflow
except:
    loc = "/usr/local/apps/ecflow/current/lib/python2.7/site-packages/ecflow"
    sys.path.append(loc)
    import ecflow

"""
another ecFlow python client example

######### environment setup:
virtualenv venv ; source ./venv/bin/activate; pip install fusepy

python $0 $ECF_HOST $ECF_PORT mnt_point

https://github.com/libfuse/libfuse

commands="   autocancel   
clock        complete     cron         date         day          defstatus    
edit         endfamily    endsuite     endtask      event        extern       
family       inlimit      label        late         limit        meter        
        repeat       suite        task              time         
today        trigger      "

# status    OK, .XXX file
# suspended OK, additional .sus file
# label     OK new_value
# meter     OK value
# event     OK value
# trigger  evaluate NOK
# complete evaluate NOK
# limit value OK + node_paths NOK
# job, output OK check time stamp
# remote output NOK 
# time stamp as status update NOK
# server load stat png history zombies OK
# begin

"""
FILESIZE = 10000
UPDATE = "time"
UPDATE = "always"
ECFLOW_CLIENT = "/usr/local/apps/ecflow/current/bin/ecflow_client"

ignore = ("bdmv", "inf", )

cmds = { # "log": "--log=get",
         "history": "--log=get",
         "stats": "--stats",
         "png": "--server_load",  
         "why": "--group='get; why %s'",
         # + "=%s.%s.png; " % (self.host, self.port),
         # ) + "cat %s.%s.png" % (self.host, self.port),
         "zombies": "--zombie_get" }

ECF_EXTN = "sms"
ECF_EXTN = "ecf"
exts = { key: None for key in cmds.keys() }
task_exts = {
    ECF_EXTN: "script", "man": "manual", "out": "jobout", "job": "job",  
    # "nop": None,         
    "att": None,
}
exts.update(task_exts)

statuses = {
            -1: "unknown",
             0: "unknown",
             1: "suspended",
             2: "complete",
             3: "queued",
             4: "submitted",
             5: "active",
             6: "aborted",
             7: "shutdown",
             8: "halted",
             9: "unknown",             }

state3 = ("unk", "que", "sub", "act", "com", "abo", "sus", "hal", "shu", "sus")

def list_dir(node):
    item = node
    res = []
    if isinstance(item, ecflow.Task):
        res.extend([ ".%s" % key for key in task_exts.keys() ])
    else: res = [ ".att", ]

    name = item.name()
    if node.is_suspended(): 
        # res.extend([ name + ".sus"]) # another item???
        sus = ".sus"
    else: sus = ""
    state = ".%03s" % item.get_state()
    res.extend([ state[:4] + sus])

    for item in node.nodes:
        name = item.name()
        if node.is_suspended(): 
            # res.extend([ name + ".sus"]) # another item???
            sus = ".sus"
        else: sus = ""
        state = ".%03s" % item.get_state()
        # res.extend([ name + state[:4] + sus])
        
        # if isinstance(item, ecflow.Task):
        #     res.extend([ name + ".%s" % key for key in task_exts.keys() ])
        # else: res.extend([ name, name + ".att"])

    return res

def list_att(node):
        import pprint

        if node is None: return "none"
        attr = dict()

        kinds = { ecflow.Defs: "defs",
                  ecflow.Suite: "suite",
                  ecflow.Family: "family",
                  ecflow.Task: "task",
                  ecflow.Alias: "alias", }

        try: attr['kind'] = kinds[type(node)]
        except: print "what?", type(node); attr['kind'] = "unknown"

        if isinstance(node, ecflow.Defs):
            attr['gvar'] = []
            attr['edit'] = []
            for item in node.server_variables:
                attr['gvar'] += [ (item.name(), item.value()) ]
            for item in node.user_variables:
                attr['edit'] += [ (item.name(), item.value())]
            pp = pprint.PrettyPrinter(indent=4)
            return pp.pformat(attr)
      
        elif isinstance(node, ecflow.Suite):            
            clock = node.get_clock()
            if clock: attr['clock']= "%s" % clock
            attr['begin']= node.begun()

        attr['status'] = "%s" % node.get_state()
        try:
            attr['update'] = "%s" % node.get_state_change_time()
        except: pass

        if node.is_suspended():
            attr['suspended'] = "true"

        if node.has_time_dependencies(): 
            attr['has_time_dependencies'] = "%s" % node.has_time_dependencies()

        defstatus = node.get_defstatus()
        if defstatus != ecflow.DState.queued: 
            attr['defstatus'] = "%s" % defstatus
            
        autocancel = node.get_autocancel()
        if autocancel: attr['autocancel']= "%s" % autocancel
        
        repeat = node.get_repeat()
        if not repeat.empty(): attr['repeat']= "%s # value:%s" % (repeat, 
                                                                  repeat.value())
    
        late = node.get_late()
        if late: attr['late']= "%s" % late

        complete_expr = node.get_complete()
        if complete_expr:
            for part_expr in complete_expr.parts:
                trig = "complete "
                if part_expr.and_expr(): trig = trig + "-a "
                if part_expr.or_expr():  trig = trig + "-o "
                attr['complete']= "%s" % trig + " %s" % \
                           part_expr.get_expression() + "\n"
        trigger_expr = node.get_trigger()
        if trigger_expr:
            for part_expr in trigger_expr.parts:
                trig = "trigger "
                if part_expr.and_expr(): trig = trig + "-a "
                if part_expr.or_expr():  trig = trig + "-o "
                attr['trigger'] = "%s" % trig + " %s" % \
                           part_expr.get_expression() + "\n"
            
        gvar = ecflow.VariableList()
        node.get_generated_variables(gvar)
        attr['gvar'] = []
        for item in gvar:
            attr['gvar'] += [ (item.name(), item.value()) ]
        attr['edit'] = [ (item.name(), item.value()) 
                         for item in node.variables ]

        addit(node.meters, attr, 'meter')
        addit(node.events, attr, 'event')
        addit(node.labels, attr, 'label')
        addit(node.limits, attr, 'limit')
        addit(node.inlimits, attr, 'inlimits')
        addit(node.times, attr, 'time')
        addit(node.todays, attr, 'today')
        addit(node.dates, attr, 'date')
        addit(node.days, attr, "day")
        addit(node.crons, attr, "cron")
        addit(node.zombies, attr, "zombies")

        pp = pprint.PrettyPrinter(indent=4)
        return pp.pformat(attr) + "\n"

def set_att(fpath, dct, att=('st_size', ), dft=FILESIZE, ext="job"):
    print "set_att", fpath
    # if ext == "out": fpath = fpath.replace(".job", ".")
    if os.path.exists(fpath):
        print "'FOUND", fpath, "\n", os.lstat(fpath)
        for key in att: 
            if "uid" in att or "gid" in att: continue
            dct[key] = getattr(os.lstat(fpath), key)
            print fpath, key, dct[key]
    else: dct['st_size'] = FILESIZE

def remote_file(node, fname, gvar, res):
    print "####### REMOTE"
    # gvar = ecflow.VariableList()
    for item in gvar:
        print item
        if item.name() == "ECF_JOBOUT":
            fname = item.value()
            break

    import paramiko, base64
    ssh = paramiko.SSHClient()
    HOST = None
    if "ccb" in res: HOST = "ccb-il2"
    if "cca" in res: HOST = "cca-il2"
    if not HOST: return res
    #key = paramiko.RSAKey(data=base64. decodestring(' '))
    #ssh.get_host_keys().add(HOST, 'ssh-rsa', key)
    ssh.set_missing_host_key_policy(
        paramiko.AutoAddPolicy())
    ssh.connect(HOST, username="emos") # , password=)
    cmd = "ls %s.running && tail -20 %s.running || " % (
        fname, fname) + "tail -20 %s" % fname
    stdin, stdout, stderr = ssh.exec_command(cmd)
    print "#MSG: live output: ", HOST, cmd
    for line in stdout:
        print line,
        res += "%s" % line
    return res

def addit(array, cont, name):
        rc = []
        for item in array:
            load = "%s" % item
            if name == "label": load += " # value:%s" % item.new_value()
            elif name in ("meter", "event"): 
                load += " # value:%s" % item.value()
            elif name in ("limit",): 
                if item.value() > 0:
                    load += " # value:%s" % item.value()
                    print item.node_paths
                    for idx, val in enumerate(item.node_paths):
                        print idx, val
                        load += " %s " % val # ???
            rc.append(load)
        if len(rc) > 0: cont[name] = rc

def trunc(res, size, offset):
    print "offset", offset, size
    if len(res) > size: 
        return "#TRUNCATED\n" + res[offset:offset+size-30] + "\n"
    return res + "\n"

class FuseEcflow(LoggingMixIn, Operations):
    '''
    A simple Ecflow python client example
    '''

    def __init__(self, host="localhost", port=31415, path='.'):
        self.client = ecflow.Client(host, port)
        self.client.sync_local()
        self.host = host
        self.port = port
        self.update = int(time.strftime("%H%M"))
        self.defs = self.client.get_defs()
        self.root = path
        print "#MSG: connected to %s " % host + port
        if 0: 
            for s in self.defs.suites: print s.name(),
            print

    def chmod(self, path, mode):
        raise FuseOSError(ENOENT)

    def chown(self, path, uid, gid):
        raise FuseOSError(ENOENT)

    def create(self, path, mode):
        raise FuseOSError(ENOENT)

    def destroy(self, path): pass

    def server_cmd(self, ext, run=1):
        if not ext: return ""
        if not ext in cmds.keys():
            print "server cmd: what?", ext
            return ""
        cmd = ECFLOW_CLIENT + " --port %s --host %s " % (self.port, self.host
          ) + cmds[ext]
        if ext == "png":
            if not run: return " " * FILESIZE
            cmd += "=%s.%s.png; " % (self.host, self.port)

        import commands
        print cmd
        (rc, res) = commands.getstatusoutput( cmd )

        if ext == "png": 
             (rc, res) = commands.getstatusoutput("cat %s.%s.png" % (
                 self.host, self.port))            
        return res

    def getattr(self, path, fh=None):
        uid, gid, pid = fuse_get_context()
        print "getattr", path, fh

        cur = os.lstat(".")
        # print "cur", cur
        st = dict((key, getattr(cur, key)) for key in (
            'st_size', 'st_gid', 'st_uid',
            'st_mode', 'st_mtime', 'st_atime', 'st_ctime',        ))

        if path == '/':
            st['st_mode']  = 0      # (protection bits)
            st['st_ino']   = 0      # (inode number)
            st['st_dev']   = 0      # (device)
            st['st_nlink'] = 0      # (number of hard links)
            # st['st_uid']   = 500    # (user ID of owner)
            # st['st_gid']   = 500    # (group ID of owner) 
            st['st_size']  = 0      # (size of file, in bytes)
            st['st_atime'] = 0      # (time of most recent access)
            st['st_mtime'] = 0      # (time of most recent content modification)
            st['st_ctime'] = 0      # (time of most recent metadata change)

            st['st_mode'] = (S_IFDIR | 0755)
            st['st_nlink']=2
            return st

        if '.' in path: 
            st['st_mode'] = (S_IFREG | 0444)
            ext = ""
            try:
                path, ext = path.split(".")
                if "@" in path: node = self.client.get_defs()
                else: node = self.client.get_defs().find_abs_node(str(path))
            except Exception as e: print e; node = None; return st

            if ext == "att":        
                st['st_size'] = len(list_att(node))

            elif "@" in path:
                res = self.server_cmd(ext, 0)
                st['st_size'] = len(res)

            elif ext in ("job", "out"):
                st['st_size'] = FILESIZE
                gvar = ecflow.VariableList()
                node.get_generated_variables(gvar)
                item = None
                for item in gvar:
                    if item.name() == "ECF_JOB": break
                if item:
                    fname = item.value()
                    if ext == "out": fname = fname.replace(".job", ".")
                    set_att(fname, st, ('st_size', 'st_atime', 'st_mtime',
                                            'st_ctime'),
                                ext=ext)

        elif 1:
            st['st_mode'] = (S_IFDIR | 0444)
            st['st_size'] = 1

        else:
            raise FuseOSError(ENOENT)
        st['st_ctime'] = st['st_mtime'] = st['st_atime'] = time.time()
        return st

    def mkdir(self, path, mode):
        raise FuseOSError(ENOENT)

    def refresh(self):
        curr = int(time.strftime("%H%M"))
        if curr - self.update > 5 or UPDATE == "always":
            self.client.sync_local()
            self.update = curr

    def read(self, path, size, offset, fh):    
        ext = None # "nop"
        if "." in path: 
            path, ext = path.split(".")

        self.refresh()

        print "read", path, size, offset, fh, ext
        if "@" in path and ext != "att":
            res = self.server_cmd(ext)
            return trunc(res, size, offset)

        if ext in state3: 
            node = self.client.get_defs().find_abs_node(str(path))
            suspended = ""
            if node.is_suspended(): suspended = " # node is suspended"
            return "node %s state is %s%s\n" % (node.get_abs_node_path(), 
                                                node.get_state(), suspended)
        elif ext in ignore: return "-ignored-"
        elif ext and not ext in exts.keys(): 
            print "what?", ext; return "-empty-"

        elif ext in ("job", "out"):
            node = self.client.get_defs().find_abs_node(str(path))
            gvar = ecflow.VariableList()
            node.get_generated_variables(gvar)
            fname = path
            for item in gvar:
                if item.name() == "ECF_JOB":
                    fname = item.value()
                    break

            if os.path.exists(fname + ".sub"):
                with open(fname + ".sub") as f:
                    res = f.read()

            if ext == "out": fname = fname.replace(".job", ".")
            if os.path.exists(fname):
                if ext == "out": 
                    jobtime = os.path.getmtime(item.value())
                    outtime = os.path.getmtime(fname)
                    if jobtime > outtime:
                        res += "output older than jobfile, %s\n" % item.value()
                        try: res += remote_file(node, fname, gvar, res)
                        except Exception as e: print e
                        return trunc(res, size, offset)

                f = open(fname)
                f.seek(offset, 0)
                buf = f.read(size)
                f.close()
                return buf
            else: 
                try: res += remote_file(node, fname, gvar, res)
                except Exception as e: print e
                return trunc(res, size, offset)
                
            try: res = "%s" % self.client.get_file(str(path), exts[ext])
            except  Exception as e: print e; res = "%s" % e 
            return trunc(res, size, offset)

        elif ext in ("why", ):
            print "##############", ext
            cmd = ECFLOW_CLIENT + " --port %s --host %s " % (self.port, self.host
            ) + cmds[ext] % path

            import commands
            print cmd
            (rc, res) = commands.getstatusoutput( cmd )
            return trunc(res, size, offset)

        elif ext in (ECF_EXTN, "man"):
            print "##############", ext
            res = "%s" % self.client.get_file(str(path), exts[ext])
            return trunc(res, size, offset)

        elif "@" in path: node = self.client.get_defs()
        else: node = self.client.get_defs().find_abs_node(str(path))
        res = ""

        if path == '/':
            for s in self.defs.suites: res += "%s " % s.name()
            return res

        elif node is None: return "-empty-node is none"

        elif ext == "att": 
            res = "%s" % list_att(node)
            return trunc(res, size, offset)
            
        elif 1:
            for s in node.nodes: res += "%s\n" % s.name()
            return res

        else: raise FuseOSError(ENOENT)

        f = open(path)
        f.seek(offset, 0)
        buf = f.read(size)
        f.close()
        return buf

    def readdir(self, path='/', fh=None):
        ext = None
        if "." in path: 
            path, ext = path.split(".")
        if ext and not ext in exts.keys(): 
            print "readdir: what?", ext; 
            return [ "/" ]

        print "readdir", path, fh, path, ext

        self.refresh()

        if ext: #  != "nop": 
            return [ ".", "..", "ok", path.replace("/", "_"), ext ]
        else: 
            # if path[-1] == '/': path[-1] = ''
            node = self.client.get_defs().find_abs_node(str(path))
        res = []

        if path == '/':
            node = self.client.get_defs()
            nick = '%s@%s' % (self.host, self.port)
            res = ['.', '..', nick + "." + 'att' ]
            res += [ nick + ".%s" % key for key in cmds.keys() 
                     if key != "why" ]
            res += [ nick + '.%s' % ( node.get_server_state()) ]
            for s in node.suites:
                sus = ""
                if s.is_suspended(): sus = ".sus"
                state = ".%.3s" % s.get_state()
                res += [ "%s" % s.name(), s.name() + state + sus ]
            return res

        elif node is None: 
            print "#ERR: not found", path 
            return [ "..", "." ]
            # raise BaseException()
            # return ["-empty-%s" % path.replace("/", "_") ]

        else: 
            res = ['.', '..', ".why" ]
            res += [ "%s" % n.name() for n in node.nodes ] 
            res += list_dir(node)
            return res

    def readlink(self, path):
        raise FuseOSError(ENOENT)

    def rename(self, old, new):
        raise FuseOSError(ENOENT)   

    def rmdir(self, path):
        raise FuseOSError(ENOENT)   

    def symlink(self, target, source):
        raise FuseOSError(ENOENT)   

    def truncate(self, path, length, fh=None):
        raise FuseOSError(ENOENT)   

    def unlink(self, path):
        raise FuseOSError(ENOENT)   

    def utimens(self, path, times=None):
        raise FuseOSError(ENOENT)   

    def write(self, path, data, offset, fh):
        raise FuseOSError(ENOENT)   

if __name__ == '__main__':
    if len(argv) < 4:
        print('usage: %s <host> <port> <mountpoint>' % argv[0])
        exit(1)

    fuse = FUSE(FuseEcflow(argv[1], argv[2]), argv[3], 
                foreground=True, nothreads=True, 
                # allow_root=True, # echo user_allow_other >> /etc/fuse.conf
                # allow_other=True,
    )

"""
python ecflow_fuse.py $ECF_NODE $ECF_PORT eod3

ecflow_client  --server_load --port $ECF_PORT --host $ECF_NODE
xdg-open ${ECF_NODE}.${ECF_PORT}.png

dircolors -p 
LS_COLORS+="*.abo=41:*.com=43:*.sus=01;45:*.que=44:*.sub=01;46:*.unk=47:*.act=01;42:"

"""

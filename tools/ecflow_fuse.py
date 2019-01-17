#!/usr/bin/env python
from __future__ import print_function
import os
import sys
import time
import getopt
from errno import ENOENT
from stat import S_IFDIR, S_IFREG, S_IFBLK
from sys import argv, exit
try:
  from fuse import FUSE, Operations, LoggingMixIn, FuseOSError, fuse_get_context
except:
  raise Exception("#ERR: fuse is needed\n pip install --user fuse ")

try:
    import ecflow
except:
    loc = "/usr/local/apps/ecflow/current/lib/python2.7/site-packages/ecflow"
    sys.path.append(loc)
    import ecflow

try:
    import cdp
    CDP = True
except:
    CDP = False

README = """
NODE.acr  # STATUS: act que sus sub abo
NODE.att  # attributes

SERVER.log  # mounted log file
SERVER.stats  # info
SERVER.ping
SERVER.zombies

# client:
ARG="--ping" ./.exe
ARG="--stats" ./.exe

"""

sms_status = {
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
    9: "unknown",
}

sms_flags = (
    'FLAG_FORCE_ABORT',
    'FLAG_USER_EDIT',
    'FLAG_TASK_ABORTED',
    'FLAG_EDIT_FAILED',
    'FLAG_CMD_FAILED',
    'FLAG_NO_SCRIPT',
    'FLAG_KILLED',
    'FLAG_MIGRATED',
    'FLAG_LATE',
    'FLAG_MESSAGE',
    'FLAG_BYRULE',
    'FLAG_QUEUELIMIT',
    'FLAG_WAIT',
    'FLAG_LOCKED',
    'FLAG_ZOMBIE',
    'FLAG_NOT_ENQUEUED',
    'FLAG_MAX',
)

smsrepeat = ('REPEAT_DAY', 'REPEAT_MONTH', 'REPEAT_YEAR',
             'REPEAT_INTEGER', 'REPEAT_STRING', 'REPEAT_FILE', 'REPEAT_DATE',
             'REPEAT_ENUMERATED', 'REPEAT_MAX')

sms_late = ('LATE_SUBMITTED', 'LATE_ACTIVE', 'LATE_COMPLETE',
            'LATE_MAX',)

sms_zombies = ('ZOMBIE_SMS', 'ZOMBIE_USER', 'ZOMBIE_NET',)

sms_zomies_t = ('ZOMBIE_GET', 'ZOMBIE_FOB', 'ZOMBIE_DELETE',
                'ZOMBIE_FAIL', 'ZOMBIE_RESCUE',)

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

DEBUG = 1
FILESIZE = 10000
UPDATE = "time"
UPDATE = "always"
UPDATE_INTERVAL = 30  # minutes

ECFLOW_CLIENT = "/usr/local/apps/ecflow/current/bin/ecflow_client"
EXE_CMD = "ecflow_client --port %d --host %s ${ARG:=--help};\n"
ignore = ("bdmv", "inf", )

cmds = {  # "log": "--log=get",
    "history": "--log=get",
    "stats": "--stats",
    "png": "--server_load",
    "why": "--group='get; why %s'",
    "log": None,
    "ping": "--ping",
    # "news": "--news",
    # "url": "--url ${ECF_NAME}",
    # + "=%s.%s.png; " % (self.host, self.port),
    # ) + "cat %s.%s.png" % (self.host, self.port),
    "zombies": "--zombie_get"}

ECF_EXTN = "sms"
ECF_EXTN = "ecf"
exts = {key: None for key in cmds.keys()}
task_exts = {
    ECF_EXTN: "script",
    "man": "manual",
    "out": "jobout",
    "job": "job",
    "SUB": None, "pout": "output", "pjob": "job", "log": None,
    "why": None, "exe": None, "url": "None", "README": None,
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

def attrs(name):
    cur = os.lstat(name)
    return dict((key, getattr(cur, key)) for key in (
        'st_size', 'st_gid', 'st_uid',
        'st_mode', 'st_mtime', 'st_atime', 'st_ctime', ))


def list_dir(node):
    item = node
    res = []
    if CDP:
        if isinstance(item, cdp.sms_node):
            sms_type = {13: "definition",
                        12: "suite",
                        11: "family",
                        10: "task",
                        32: "alias", }
            if sms_type[item.type] == "task":
                res.extend([".%s" % key for key in task_exts.keys()])
            name = item.name
            state = ".%s" % sms_status[item.status][:3]
            if state == ".sus":
                state += ".%s" % sms_status[item.savestate][:3]
            res.extend([name + state])
            # item = node.kids
            # while item:
            #     name = item.name
            #     state = ".%03s" % sms_status[item.status]
            #     item = item.next
    else:
        if isinstance(item, ecflow.Task):
            res.extend([".%s" % key for key in task_exts.keys()])
        for deco in item.meters:
            res.extend([".meter.%s.%s" % (deco.name(), deco.value())])
        for deco in item.events:
            res.extend([".event.%s.%s" % (deco.name(), deco.value())])
        name = item.name()
        state = ""
        if node.is_suspended():
            state += ".sus"
        state += ".%03s" % item.get_state()

        res.extend([state[:4] + 'sus'])
    return res


def list_att_sms(item):
    import pprint
    attr = dict()
    ones = (
        'act_no',
        'clock',
        'savedstat',
        'action',
        'flags',
        'modified',
        'status',
        'alias',
        'count',
        'gain',
        'late',
        'name',
        'stime',
        'tryno',
        'autocm',
        'date',
        'restore',
        'text',
        'type'
        'wait', 'btime', 'defstatus', 'nid', 'rid', '.user')

    def add(item, key, one=0):
        if not item:
            return
        # if one or key in ones: attr[key] = item
        if key == 'flags':
            out = []
            for i, flag in enumerate(sms_flags):
                print(item, i, 'flag')
                if item & i:
                    out += [flag]
            attr[key] = (item, out)
        elif key == 'limit':
            attr[key] = {'act_no': item.act_no,
                         'max': item.limit,
                         'name': item.name,
                         'base': item.base,
                         'type': item.limittype,
                         'status': item.status,
                         'min': item.min,
                         'nid': item.nid,
                         'tasks': add(item.tasks, 'list'),
                         'unit': item.unit}

        elif key == 'repeat':
            attr[key] = {'act_no': item.act_no,
                         'end': item.end,
                         'mode': item.mode,
                         'name': item.name,
                         'start': item.start,
                         'status': item.status,
                         'step': item.step,
                         'str': add(item.str, list)}

        elif key == 'math':
            operator_s = (
                "or", "and",
                "eq", "ne",
                "lt", "le", "gt", "ge",
                "+", "-",
                "*", "/", "mod",
                "^",
                "not", "~",
                "(", ")")
            out = dict()
            if one != 0:
                out[one] = None
            if item.left:
                out['left'] = add(item.left, 'math'),
            if item.mtype == 19:
                out['name'] = item.name
            else:
                out['op'] = operator_s[item.mtype - 1]
            if item.right:
                out['right'] = add(item.right, 'math'),
            return out

        elif key in ('time', 'cron', 'today'):
            return {'hour': item.hour,
                    'iscron': item.iscron,
                    'minute': item.minute,
                    'nextime': item.nextime,
                    'lasttime': item.lasttime,
                    'relative': item.relative,
                    'repeat': item.repeat,
                    'today': item.today,
                    'weekdays': item.weekdays,
                    'next': add(item.next, key)}

        elif key in ('trigger', 'complete', ):
            attr[key] = {'math': add(item.math, 'math'),
                         'type': item.type,
                         'status': item.status, }
        elif key == 'list':
            out = []
            while item:
                out += [item.name]
                item = item.next
            return out

        elif key in ('inlimit', 'log'):
            attr[key] = add(item, 'list')

        elif key in ('gvar', 'edit', 'label', 'meter', 'event', 'limit'):
            attr[key] = dict()
            while item:
                attr[key][item.name] = item.value
                item = item.next
        else:
            attr[key] = item
    add(item.genvars, 'gvar')
    add(item.variable, 'edit')
    add(item.label, 'label')
    add(item.meter, 'meter')
    add(item.event, 'event')
    add(item.limit, 'limit')
    add(item.inlimit, 'inlimit')
    add(item.log, 'log')
    add(item.act_no, 'act_no')
    add(item.action, 'action')
    add(item.alias, 'alias')
    add(item.autocm, 'autocm')
    add(item.date, 'date')
    add(item.nid, 'nid')
    add(item.gain, 'gain')
    add(item.clock, 'clock')
    add(item.trigger, 'trigger')
    add(item.complete, 'complete')
    add(sms_status[item.defstatus], 'defstatus')
    add(item.late, 'late')
    add(item.flags, 'flags')
    add(item.repeat, 'repeat')
    # add(sms_status[item.status], 'status')
    add(item.wait, 'wait')

    if 0:
        print ("""# item.btime item.label item.meter item.restore item.stime
 item.genvars item.modified item.parent item.rid item.text item.tryno
 item.user_ptr item.complete item.event item.inlimit item.limit
 item.name item.passwd item.savedstat item.this item.type
 item.variable item.count item.flags item.kids item.log item.next
    item.repeat item.status item.time item.user item.wait""")

    pp = pprint.PrettyPrinter(indent=4)
    return pp.pformat(attr) + "\n"


def list_att(node):
    import pprint
    if node is None:
        return "none"
    if CDP:
        if type(node) == cdp.sms_node:
            return list_att_sms(node)

    attr = dict()

    ecf_type = {ecflow.Defs: "defs",
                ecflow.Suite: "suite",
                ecflow.Family: "family",
                ecflow.Task: "task",
                ecflow.Alias: "alias", }

    try:
        attr['kind'] = ecf_type[type(node)]
    except:
        print("#WAR: what?", type(node))
        attr['kind'] = "unknown"

    if isinstance(node, ecflow.Defs):
        attr['gvar'] = []
        attr['edit'] = []
        for item in node.server_variables:
            attr['gvar'] += [(item.name(), item.value())]
        for item in node.user_variables:
            attr['edit'] += [(item.name(), item.value())]
        pp = pprint.PrettyPrinter(indent=4)
        return pp.pformat(attr)

    elif isinstance(node, ecflow.Suite):
        clock = node.get_clock()
        if clock:
            attr['clock'] = "%s" % clock
        attr['begin'] = node.begun()

    attr['status'] = "%s" % node.get_state()
    try:
        attr['update'] = "%s" % node.get_state_change_time()
    except:
        pass

    if node.is_suspended():
        attr['suspended'] = "true"

    if node.has_time_dependencies():
        attr['has_time_dependencies'] = "%s" % node.has_time_dependencies()

    defstatus = node.get_defstatus()
    if defstatus != ecflow.DState.queued:
        attr['defstatus'] = "%s" % defstatus

    autocancel = node.get_autocancel()
    if autocancel:
        attr['autocancel'] = "%s" % autocancel

    repeat = node.get_repeat()
    if not repeat.empty():
        attr['repeat'] = "%s # value:%s" % (repeat,
                                            repeat.value())

    late = node.get_late()
    if late:
        attr['late'] = "%s" % late

    complete_expr = node.get_complete()
    if complete_expr:
        for part_expr in complete_expr.parts:
            trig = "complete "
            if part_expr.and_expr():
                trig = trig + "-a "
            if part_expr.or_expr():
                trig = trig + "-o "
            attr['complete'] = "%s" % trig + " %s" % \
                part_expr.get_expression() + "\n"
    trigger_expr = node.get_trigger()
    if trigger_expr:
        for part_expr in trigger_expr.parts:
            trig = "trigger "
            if part_expr.and_expr():
                trig = trig + "-a "
            if part_expr.or_expr():
                trig = trig + "-o "
            attr['trigger'] = "%s" % trig + " %s" % \
                part_expr.get_expression() + "\n"

    gvar = ecflow.VariableList()
    node.get_generated_variables(gvar)
    attr['gvar'] = []
    for item in gvar:
        attr['gvar'] += [(item.name(), item.value())]
    attr['edit'] = [(item.name(), item.value())
                    for item in node.variables]

    addit(node.meters, attr, 'meter')
    addit(node.events, attr, 'event')
    addit(node.labels, attr, 'label')
    addit(node.limits, attr, 'limit')
    addit(node.inlimits, attr, 'inlimits')
    if node.has_time_dependencies():
        addit(node.time, attr, 'time')
        addit(node.times, attr, 'cron')
        addit(node.todays, attr, 'today')
        addit(node.dates, attr, 'date')
        addit(node.days, attr, "day")
        addit(node.crons, attr, "cron")
    addit(node.zombies, attr, "zombies")

    pp = pprint.PrettyPrinter(indent=4)
    return pp.pformat(attr) + "\n"

olds = dict()
CYCLE = "45r1"
for key in ("o", "law", "mc", "mofc", "tc3", "compo", "ecompo",
            "e_" + CYCLE, "elaw_" + CYCLE, "emc_" + CYCLE, ):
    olds["/%s/" % key] = "/%s.old/" % key


def list_used_variables(node, ARGS=None):
    """ thanks Avi
    https://software.ecmwf.int/wiki/pages/viewpage.action?pageId=53513079
    """
    if ARGS is None:
        ARGS = object()
        ARGS.var_name = None
        ARGS.not_value = None

    # make a list of all nodes up the parent hierarchy
    parent_hierarchy = []
    parent_hierarchy.append(node)
    parent = node.get_parent()
    while parent:
        parent_hierarchy.append(parent)
        parent = parent.get_parent()
    # now reverse the list
    parent_hierarchy.reverse()
    # now create a map of all used variables going down the hierarchy.
    # Allow variable lower down the hierarchy to override parent variables
    variable_map = {}
    for node in parent_hierarchy:
        for var in node.variables:
            variable_map[var.name()] = var.value()
    # finally print the used variables
    if ARGS.var_name:
        if ARGS.not_value:
            # use exact match for key
            for key in variable_map:
                if ARGS.var_name == key:
                    if ARGS.not_value != variable_map[key]:
                        print("edit " + key + " '" + variable_map[key] + "'")
        else:
            # use substring match for variable name
            for key in variable_map:
                if ARGS.var_name in key:
                    print("edit " + key + " '" + variable_map[key] + "'")
    else:
        for key in variable_map:
            print("edit " + key + " '" + variable_map[key] + "'")


def set_att(fpath, dct, att=('st_size', ), dft=FILESIZE, ext="job"):
    # if DEBUG: print ("set_att", fpath)

    if os.path.exists(fpath):
        print("'FOUND", fpath, "\n", os.lstat(fpath))
        for key in att:
            if "uid" in att or "gid" in att:
                continue
            dct[key] = getattr(os.lstat(fpath), key)
            print(fpath, key, dct[key])

    elif ".old" not in fpath:
        for key in olds.keys():
            if key in fpath:
                return set_att(fpath.replace(key, olds[key]),
                               dct, att, dft, ext)

    dct['st_size'] = FILESIZE
    return dct


def remote_file(node, fname, res):
    if DEBUG:
        print("####### REMOTE")
    if CDP:
        if type(node) == cdp.sms_node:
            return "SMS!!!"

    if node:
        gvar = ecflow.VariableList()
        node.get_generated_variables(gvar)
        for item in gvar:
            # if DEBUG: print(item)
            if item.name() == "ECF_JOBOUT":
                fname = item.value()
                break

    import paramiko
    import base64
    ssh = paramiko.SSHClient()
    HOST = None
    if not HOST:
        return res
    #key = paramiko.RSAKey(data=base64. decodestring(' '))
    #ssh.get_host_keys().add(HOST, 'ssh-rsa', key)
    ssh.set_missing_host_key_policy(
        paramiko.AutoAddPolicy())
    ssh.connect(HOST, username=os.getenv("USER"))  # , password=)
    cmd = "ls %s.running && tail -20 %s.running || " % (
        fname, fname) + "tail -20 %s" % fname
    stdin, stdout, stderr = ssh.exec_command(cmd)
    if DEBUG:
        print("#MSG: live output: ", HOST, cmd)
    for line in stdout:
        #if DEBUG:
        #    print(line,)
        res += "%s" % line
    return res


# def remote_log_file(host, port, kind):
def remote_log_file(node, fname, res):
    if DEBUG:
        print("####### REMOTE")
    if CDP:
        if type(node) == cdp.sms_node:
            return "SMS!!!"

    HOST, PORT = fname.replace(".log", "").split("@")
    keys = {"3141": "/tmp/localhost.ecf.log",
            }
    ssh.set_missing_host_key_policy(
        paramiko.AutoAddPolicy())
    ssh.connect(HOST, username=os.getenv("USER"))  # , password=)
    cmd = "tail -20 %s " % (keys[port])
    stdin, stdout, stderr = ssh.exec_command(cmd)
    if DEBUG:
        print("#MSG: live output: ", HOST, cmd)
    for line in stdout:
        #if DEBUG:
        #    print(line,)
        res += "%s" % line
    return res


def addit(array, cont, name):
    rc = []
    for item in array:
        load = "%s" % item
        if name == "label":
            load += " # value:%s" % item.new_value()
        elif name in ("meter", "event"):
            load += " # value:%s" % item.value()
        elif name in ("limit", ):
            if item.value() > 0:
                load += " # value:%s" % item.value()
                #if DEBUG:
                #    print(item.node_paths)
                for idx, val in enumerate(item.node_paths):
                    print(idx, val)
                    load += " %s " % val  # ???
        rc.append(load)
    if len(rc) > 0:
        cont[name] = rc


def trunc(res, size, offset):
    # if DEBUG: print("offset", offset, size)
    if len(res) > size:
        return "#TRUNCATED\n" + res[offset:offset + size - 30] + "\n"
    return res + "\n"


class FuseEcflow(LoggingMixIn, Operations):
    '''
    A simple Ecflow python client example
    '''

    def __init__(self, host="localhost", port=31415, path='.'):
        self.host = host
        self.port = int(port)
        self.update = int(time.strftime("%H%M"))
        self.root = path
        self.top = None
        self.client()
        #if DEBUG:
        #    print("#MSG: connected to %s " % host + port)

    def client(self):
        self.client = ecflow.Client(self.host, self.port)
        self.client.sync_local()
        self.defs = self.client.get_defs()
        # print (self.defs)

    def get_node(self, path):
        black = ("/ecflow_client", "/autorun", "/BDMV", "/AACS", "BDSVM", 
                 "/RCS")
        for key in black:
            if path == key or key in path:
                return None
        if '@' in path:
            return self.client.get_defs()
        elif '/' == path:
            return self.client.get_defs()
        node = self.client.get_defs().find_abs_node(str(path))
        if node is None:
            raise BaseException(path)
        return node

    def chmod(self, path, mode):
        raise FuseOSError(ENOENT)

    def chown(self, path, uid, gid):
        raise FuseOSError(ENOENT)

    def create(self, path, mode):
        raise FuseOSError(ENOENT)

    def destroy(self, path): pass

    def server_cmd(self, ext, run=1):
        if not ext:
            return ""
        elif not ext in cmds.keys():
            print("#WAR: server cmd: what?", ext)
            return ""
        elif ext in ("log", ):
            res = ""
            # res = remote_log_file(node, fname, res)
            # HOST, PORT = fname.replace(".log", "").split("@")
            keys = {3141: "/tmp/localhost.ecf.log",}
            if self.port in keys:
                res = remote_file(None, keys[self.port], self.host)
            return res  # trunc(res, size, offset)
        elif ext == "png" and not run:
            return " " * FILESIZE
        elif cmds[ext] is None:
            return ""
        cmd = ECFLOW_CLIENT + " --port %d --host %s " % (
            self.port, self.host) + cmds[ext]
        #if DEBUG:
        #    print(cmd)

        import commands
        (rc, res) = commands.getstatusoutput(cmd)

        if ext == "png":
            (rc, res) = commands.getstatusoutput("xdg-open %s.%d.png" % (
                self.host, self.port))
        return res

    def getattr(self, path, fh=None):
        uid, gid, pid = fuse_get_context()
        # if DEBUG: print( "getattr", path, fh)

        st = attrs('.')

        if path == '/':
            st['st_mode'] = 0      # (protection bits)
            st['st_ino'] = 0      # (inode number)
            st['st_dev'] = 0      # (device)
            st['st_nlink'] = 0      # (number of hard links)
            st['st_size'] = 0      # (size of file, in bytes)
            st['st_mode'] = (S_IFDIR | 0750)
            st['st_nlink'] = 2
            return st

        st['st_ctime'] = st['st_mtime'] = st['st_atime'] = time.time()

        if '.' in path:
            st['st_mode'] = (S_IFREG | 0444)
            ext = ""
            try:
                splitted = path.split('.')
                path = splitted[0]
                ext = splitted[-1]
                # if "@" in path: node = self.client.get_defs()
                # else: node = self.client.get_defs().find_abs_node(str(path))
                node = self.get_node(path)

            except Exception as e:
                print("#ERR:", e)
                node = None  # return st

            if ext == "att":
                try:
                    st['st_size'] = len(list_att(node))
                except:
                    st['st_size'] = 0

            elif "README" in path or ".meter." in path or ".event" in path:
                st['st_mode'] = (S_IFREG | 0444)
                st['st_size'] = FILESIZE

            elif ext in ("exe", "url", ):
                st['st_mode'] = (S_IFREG | 0700)

            elif "@" in path:
                res = self.server_cmd(ext, 0)
                st['st_size'] = len(res)

            elif (ext in state3 or "sus" in ext) and node:
                tim = node.get_state_change_time()
                st = set_att(path + ".%s" % ext, st,  # dict(),  # attrs(),
                             ('st_size', 'st_atime', 'st_mtime', 'st_ctime'),
                             ext=tim)

            elif ext in ("job", "out", "SUB", "pjob", "pout"):
                st['st_size'] = FILESIZE
                item = None
                if CDP:
                    if type(node) == cdp.sms_node:
                        pass
                else:
                    gvar = ecflow.VariableList()
                    node.get_generated_variables(gvar)
                    for item in gvar:
                        if item.name() == "ECF_JOB":
                            break
                if item:
                    fname = item.value()
                    if ext == "out":
                        fname = fname.replace(".job", '.')

                    elif ".job0" in fname:
                        for key in olds.keys():
                            if key in fname:
                                fname = fname.replace(key, olds[key]).replace(
                                    ".job0", '.job1')

                    elif ext == "pout":
                        for key in olds.keys():
                            if key in fname:
                                fname.replace(key, olds[key])
                                break
                        fname = fname.replace(".job", '.')

                    elif ext == "SUB":
                        if "job0" in fname:
                            fname.replace("job0", "job1")
                            for key in olds.keys():
                                if key in fname:
                                    fname.replace(key, olds[key])
                                    break
                        fname += ".SUB"

                    elif ext == "pjob":
                        for key in olds.keys():
                            if key in fname:
                                fname.replace(key, olds[key])
                                break
                        fname = fname.replace(".job", '.')
                    st = set_att(fname, st, ('st_size', 'st_atime', 'st_mtime',
                                             'st_ctime'),
                                 ext=ext)

        elif 1:
            st['st_mode'] = (S_IFDIR | 0444)
            st['st_size'] = 1

        else:
            raise FuseOSError(ENOENT)
        return st

    def mkdir(self, path, mode):
        raise FuseOSError(ENOENT)

    def refresh(self):
        curr = int(time.strftime("%H%M"))
        if curr - self.update > UPDATE_INTERVAL or UPDATE == "always":
            self.client.sync_local()
            self.update = curr

    def read(self, path, size, offset, fh):
        ext = None  # "nop"
        if len(path) > 2 and '.' in path:
            res = path.split('.')
            path = res[0]
            ext = res[1]

        self.refresh()

        node = None
        res = ''
        # if DEBUG: print('read', path, size, offset, fh, ext)
        if '@' in path and ext != 'att':
            res = self.server_cmd(ext)
            return trunc(res, size, offset)

        if ext in state3 or "sus" in ext:
            # node = self.client.get_defs().find_abs_node(str(path))
            node = self.get_node(path)
            suspended = ''
            if node.is_suspended():
                suspended = ' # node is suspended'

            tim = node.get_state_change_time()
            st = set_att(path + ".%s" % ext, dict(),  # attrs(),
                         ('st_size', 'st_atime', 'st_mtime', 'st_ctime'),
                         ext=tim)
            return 'node %s state is %s%s # last update: %s\n' % (
                node.get_abs_node_path(),
                node.get_state(), suspended, tim)

        elif ext in ignore:
            return '-ignored-'

        elif "README" in path:
            st = attrs('.')
            st['st_mode'] = (S_IFREG | 0444)
            st['st_size'] = FILESIZE
            return README

        elif ext == "att":
            node = self.get_node(path)
            # print(node)
            res = "%s" % list_att(node)
            return trunc(res, size, offset)

        elif ext and not ext in exts.keys():
            print("#WAR: read what?", ext)
            return "-empty-"

        elif ext in ("exe", ):
            print("#exe", path, ext)
            return EXE_CMD % (self.port, self.host)

        elif ext in ("url", ):
            print("#url", path, ext)
            return EXE_CMD % (self.port, self.host)

        elif ext in ("job", "out", "SUB", "pout", "pjob"):
            if ext in ("pout", "pjob"):
                if ".0" in path:
                    path = path.replace(".0", ".1")
            gvar = None
            node = self.get_node(path)
            if CDP and type(node) == cdp.sms_node:
                pass
            else:
                gvar = ecflow.VariableList()
                node.get_generated_variables(gvar)
                fname = path
                for item in gvar:
                    if item.name() == "ECF_JOB":
                        fname = item.value()
                        break

            subfile = None
            if ".job" in fname:
                subfile = fname + ".sub"
            elif ".SUB" in fname:
                subfile = fname.replace(".SUB", ".sub")
            if os.path.exists(subfile):
                with open(fname + ".sub") as f:
                    res = f.read()

            if ext == "out":
                fname = fname.replace(".job", ".")
            if os.path.exists(fname):
                if ext == "out":
                    jobtime = os.path.getmtime(item.value())
                    outtime = os.path.getmtime(fname)
                    if jobtime > outtime:
                        res += "output older than jobfile, %s\n" % item.value()
                        try:
                            res += remote_file(node, fname, res)
                        except Exception as e:
                            print("#ERR:", e)
                        return trunc(res, size, offset)

                f = open(fname)
                f.seek(offset, 0)
                buf = f.read(size)
                f.close()
                return buf

            elif ext in ("xxout", ):
                try:
                    res += remote_file(node, fname, res)
                except Exception as e:
                    print("#ERR:", e)
                return trunc(res, size, offset)

            elif ext in ("log", ):
                # try:
                res = remote_log_file(node, fname, res)
                # except Exception as e:
                # print(e)
                return trunc(res, size, offset)

            if ext in ("out", "man", "ecf", "job", ):
                try:
                    res = "%s" % self.client.get_file(str(path), exts[ext])
                except Exception as e:
                    print("#ERR:", e)
                    res = "%s" % e
            return trunc(res, size, offset)

        elif ext in ("why", ):
            print("##############", ext)
            cmd = ECFLOW_CLIENT + " --port %d --host %s " % (
                self.port, self.host) + cmds[ext] % path
            if CDP and type(node) == cdp.sms_node:
                return "sms!!!"

            import commands
            (rc, res) = commands.getstatusoutput(cmd)
            return trunc(res, size, offset)

        elif ext in (ECF_EXTN, "man"):
            print("##############", ext)
            if CDP and type(node) == cdp.sms_node:
                return "sms!!!"

            res = "%s" % self.client.get_file(str(path), exts[ext])
            return trunc(res, size, offset)

        else:
            node = self.get_node(path)
        res = ""

        if path == '/':
            if self.top:  # SMS
                item = self.top.kids
                while item:
                    res += "%s " % item.name
                    item = item.next
                return res
            for s in self.defs.suites:
                res += "%s " % s.name()  # ECFLOW
            return res

        elif ext == "exe":
            if self.top:
                return "SMS!!!"
            return "ecflow_client --port ${ECF_PORT:=31415}" + \
                " --host ${ECF_HOST:=localhost} ${ARG:=--help}"

        elif node is None:
            return "-empty-node is none"

        elif 1:
            for s in node.nodes:
                res += "%s\n" % s.name()
            return res

        else:
            raise FuseOSError(ENOENT)

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
            if DEBUG:
                print("#readdir: what?", ext)
            return ["/"]
        # if DEBUG: print ("readdir", path, fh, path, ext)

        self.refresh()

        if ext:
            return [".", "..", "ok",
                    path.replace("/", "_"), ext]
        # node = self.client.get_defs().find_abs_node(str(path))
        node = self.get_node(path)

        if node is None:
            print("#WAR: node is None", path)
            return ['.', '..', ]

        elif path == '/':
            nick = '%s@%d' % (self.host, self.port)
            if self.top:
                res = []
                res += [nick + '.%s' % (sms_status[node.status])]
                s = node.kids
                while s:
                    sus = ""
                    state = ".%.3s" % sms_status[s.status]
                    res += ["%s" % s.name, s.name + state + sus]
                    s = s.next
            else:
                res = ['.', '..', nick + "." + 'att',
                       "ecflow_client.exe", "README"]
                res += [nick + ".%s" % key for key in cmds.keys()
                        if key != "why"]
                res += [nick + '.%s' % (node.get_server_state())]
                for s in node.suites:
                    sus = ""
                    if s.is_suspended():
                        sus = ".sus"
                    state = ".%.3s" % s.get_state()
                    res += ["%s" % s.name(), s.name() + state + sus]

        elif self.top:
            res = ['.', '..', ".att", ]
            item = node.kids
            while item:
                res += ["%s" % item.name]
                res += list_dir(item)
                item = item.next
        else:
            res = ['.', '..', ".att", ]
            res += ["%s" % n.name() for n in node.nodes]
            res += ["%s.%.3s" % (n.name(), n.get_state()) for n in node.nodes]
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

NOCDP = 0
source = "/tmp"
try:
    sys.path.append(source)
    import cdp
except:
    NOCDP = 1
    print("###WAR: NO_COP")


class FuseSMS(FuseEcflow):

    def __init__(self, host="localhost", port=31415, path='.'):
        super(FuseSMS, self).__init__(host, port, path)

    def client(self):
        if NOCDP:
            print("NOCDP!")
            return
        rc = cdp.new_int()
        c = cdp.sms_node()
        tree = cdp.sms_info_pointed(c)
        os.putenv("SMS_PROG", "%d" % self.port)
        os.environ['SMS_PROG'] = "%d" % self.port
        cdp.cdp_init0()

        user = os.getenv("USER")
        tout = 60
        passw = "1"
        self.num = -1
        rc = cdp.new_int()
        c = cdp.sms_node()
        s = cdp.sms_info_pointed(c)
        self.handle = None
        if 0:
            cdp.sms_client_login(
                self.host, user, passw, tout, self.num, s, rc)
        if not self.top:
            self.top = cdp.sms_cdp_client_login(
                self.host, user, passw, tout, self.port)

    def get_node(self, path):
        if len(path) > 1 and path[-1] == '/':
            path = path[:-1]
            # print(path)
        # print ("find???", path, self.host, self.port, )
        if "@" in path:
            return cdp.sms_node_find('/', self.top)
        node = cdp.sms_node_find(str(path), self.top)
        if not node:
            print("#ERR", path, self.host, self.port, "NOT found")
        return node

    def refresh(self):
        if NOCDP:
            print("#NOCDP!")
            return
        curr = int(time.strftime("%H%M"))
        if curr - self.update > UPDATE_INTERVAL or UPDATE == "always":
            # rc = cdp.sms_client_news( self.handle, self.num );
            # cdp.sms_cdp_update(self.handle)
            self.client()
            self.update = curr


def usage():
    print('usage: %s <host> <port> <mountpoint>' % argv[0])
    print('usage: %s -h <host> -p <port> -m <mountpoint> -s' % argv[0])

if __name__ == '__main__':
    ECFLOW = True
    PORT = 31415
    HOST = "localhost"
    MOUNT = None
    import sys
    try:
        OPTS, ARGS = getopt.getopt(sys.argv[1:],
                                   "?sh:p:m:n:",
                                   ["help", "sms", "host", "port", "mount",
                                    "node", ])
    except getopt.GetoptError as err:
        print("#ERR: what?", )
        usage()
        sys.exit(2)
    for o, a in OPTS:
        if o in ("-?", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-s", "--sms"):
            ECFLOW = False
            if PORT == 31415:
                PORT == 314159
        elif o in ("-h", "--host", "-n", "--node"):
            HOST = a
        elif o in ("-p", "--port"):
            PORT = a
        elif o in ("-m", "--mount"):
            MOUNT = a

    try:
        a = int(PORT)
    except:
        print("#ERR: port shall be an integer")
        sys.exit(2)

    print(os.system("/usr/bin/fusermount -u %s" % MOUNT))
    if os.system("ping -c 1 %s" % HOST) != 0:
        print("#MSG: host appears not to be up" + HOST)
        sys.exit(2)
    elif not MOUNT:
        usage()
        sys.exit(2)
    else:
        os.system("/usr/bin/fusermount -u %s" % MOUNT)
    if not os.path.exists(MOUNT):
        os.makedirs(MOUNT)
    # elif 1: print os.stat(MOUNT)[os.stat.ST_MODE]
    # elif not os.path.isdir(MOUNT): os.mkdir(MOUNT, 0755)

    if ECFLOW:
        FUS = FUSE(FuseEcflow(HOST, PORT, MOUNT), MOUNT,
                   foreground=True, nothreads=True, )
        # allow_root=True, # echo user_allow_other >> /etc/fuse.conf
        # allow_other=True, )
    else:
        FUS = FUSE(FuseSMS(HOST, PORT, MOUNT), MOUNT,
                   foreground=True, nothreads=True, )

"""stream - class - date - expver - repres - levtype
type - param - step - time - domain - grid - resol

path fullpath queue (suite/j/o) npes threads prio mem

servers id (PK) host port kind
tasks id (PK)
task id (PK) nick FK server FK

event_type job SUB sub act com/abt out event meter id (PK) desc

"""

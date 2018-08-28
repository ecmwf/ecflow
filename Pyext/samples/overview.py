#!/usr/bin/env python
# -*- coding= UTF-8 -*-
""" tkinter example with ecflow
"""
from __future__ import with_statement, print_function
import Tkinter as tki
from Tkinter import *
import Queue
import sys
import time
import os
import pwd
from threading import Thread
try: from scrolledlist import ScrolledList
except: 
    loc = "http://infohost.nmt.edu/tcc/help/lang/python/examples/scrolledlist"
    raise Exception("wget %s/scrolledlist.py; # Thanks NMT")

loc = "/usr/local/apps/ecflow/current/lib/python2.7/site-packages/ecflow"
sys.path.append(loc)
loc = "/usr/local/lib/python2.7/site-packages/ecflow"
sys.path.append(loc)
import ecflow as ec

PROGRAM_NAME = "ecflowview-overview"
BUTTON_FONT = ('times', 12)
MONO_FONT = ('lucidatypewriter', 14, 'bold')
DEBUG = 0
COLORS = {"aborted": "red",
          "active":  "green",
          "submitted": "cyan",
          "complete": "yellow",
          "suspended": "orange",
          "queued": "blue",
          "unknwon": "grey"}
running = [True]

ONCE = False


def show_check(one):
    print(one)
    sep = ','
    if sep not in one: return
    exp = one.split(sep)
    if len(exp) != 4: return
    host, qid = exp[2], exp[3]
    check = os.getenv("CHECK_TASK", "check_task.py")
    global ONCE
    if check is not None:
        print(check +  " -n %s -j %s" % (host, qid))
    elif ONCE:
        print("#WAR: export CHECK_TASK=$HOME/bin/check_task.py; overview.py $ECF_HOST@$ECF_PORT,")
        ONCE = True


class Label(object):
    """ a class to encapsulate what was a global variable"""
    inst = None

    def __init__(self, item): Label.inst = item

    @classmethod
    def update(cls):
        if Label.inst is None:
            return
        Label.inst.set(time.strftime("%a, %d %b %Y %H:%M:%S"))


class MenuBar(tki.Frame):
    """ guess"""

    def __init__(self, parent):
        tki.Frame.__init__(self, parent)
        self.parent = parent
        self.__helpButton = self.__createHelp()
        self.__helpButton.grid(row=0, column=3)
        self.__quitButton = tki.Button(self, text='Quit', font=BUTTON_FONT,
                                       command=root.destroy)
        self.__quitButton.grid(row=0, column=1)
        self.__updateButton = tki.Button(self, text='Update', font=BUTTON_FONT,
                                         command=parent.update)
        self.__updateButton.grid(row=0, column=2)

    def quit(self, event=None):
        self.parent.quit()

    def __createHelp(self):
        mb = tki.Menubutton(self, font=BUTTON_FONT,
                            relief=tki.RAISED, text='Help')
        menu = tki.Menu(mb)
        mb['menu'] = menu
        url = "https://software.ecmwf.int/wiki/display/ECFLOW/Documentation"

        def url1(): self.__url(url=url)

        def url2(): self.__url(url="http://effbot.org/tkinterbook/")
        menu.add_command(command=url1, label="confluence tutorial?")
        menu.add_command(command=url2, label="tkinter?")
        return mb

    def __url(self, url=None):
        if url is not None:
            os.system("${BROWSER:=firefox} %s" % url)


class TaskList(tki.Frame):
    """ a class to host all tasks"""
    # WIDTH = w / 30
    # LINES = h / 10

    def __init__(self, parent, kind):
        tki.Frame.__init__(self, parent, 
                           # side="bottom", 
                           # fill="both", 
                           # expand=True,
                           # sticky="nsew",
        )
        self.WIDTH = 80
        self.LINES = 80
        self.__kind = kind
        self.__callback = None
        #self.__label = tki.Label(
        #    self, font=BUTTON_FONT, background=COLORS[kind], text=kind)
        #self.__label.grid(row=0, column=0, sticky=tki.W)
        select = tki.Button(self, text=kind,
                                 font=BUTTON_FONT,
                                 background=COLORS[kind],
                                 command=self.select)
        select.grid(row=1, column=0, sticky=tki.W)

        selectall = tki.Button(self, text="select all",
                                    font=BUTTON_FONT,
                                    # background=COLORS[kind],
                                    command=self.select_all)
        selectall.grid(row=2, column=0, sticky=tki.W)

        self.__scrolledList = ScrolledList(
            self, 
            # sticky="nsew",
            width=self.WIDTH, 
            height=self.LINES,  # selectmode=tki.EXTENDED,
            callback=self.__callback)
        self.__scrolledList.grid(row=3, column=0)

    def select_all(self):
        res = set()
        item = self.__scrolledList.listbox.get(0, tki.END)
        for one in item:
            print(one)

    def select(self):
        res = set()
        selected = self.__scrolledList.listbox.curselection()
        for i in selected:
            item = self.__scrolledList.listbox.get(i)
            res.add(item)
            for one in res: 
                show_check(one)
                
    def insert(self, path): self.__scrolledList.append(path)

    def clear(self): self.__scrolledList.clear()


class PaceKeeper():
    """ get regular updates"""
    PACE = 60

    def __init__(self, item, queue):
        self._item = item
        Thread(target=self.process, args=(queue, running)).start()

    def process(self, queue, running):
        while running:
            queue.put(self._item.update)
            time.sleep(self.PACE)

    def run(self): self.update()

    def update(self, verbose=False):
        while True:
            self._item.update()
            time.sleep(self.PACE)


class Client(object):
    """ a class to focus on client-ecFlow-server comm"""

    def __init__(self, one="localhost@31415"):
        # try: nick, hhh = one.split("-")
        # except: hhh = one; nick = None
        if "@" in one:
            host, port = one.split("@")
        # except: 
        else: host = "localhost"; port = 31415
        self.nick = one
        # if nick is None: self.nick = "%s@%s" % (host, port)
        print("# client creation", self.nick, host, port)
        self.client = ec.Client(host, port)

    def process(self, win):
        # Label.update()
        self.client.sync_local()
        defs = self.client.get_defs()
        if defs is None:
            print("# %s-%: empty content" % (self.host, self.port))
        # Label.update()
        for suite in defs.suites:
            self.process_nc(suite, win)

    def process_nc(self, node, win):
        for item in node.nodes:
            if isinstance(item, ec.Task):
                self.process_node(item, win)
            else:
                self.process_nc(item, win)

    def process_node(self, item, wins):
        for kind, win in wins.items():
            status = "%s" % item.get_state()
            if status != kind:
                continue
            change = item.get_state_change_time()
            line = "#%s:%s,%s" % (
                self.nick, item.get_abs_node_path(), change)
            try:
                import check_task as ct
                qid = str(ct.display_var(item, "qid"))
                host = ct.display_var(item, "ECF_JOB_CMD")
                line += ",%s,%s" % (host, qid)
            except: pass
            win.insert(line)

try:
    sys.path.append('/usr/local/apps/sms/lib/')
    import cdp
    NOCDP = 0
except:
    NOCDP = 1


class ClientSMS(Client):
    """ example of class derivation to connect another server-scheduler"""
    init = 0

    def __init__(self, host="localhost", port=31415, nick=None):
        if NOCDP:
            return
        if not ClientSMS.init:
            cdp.cdp_init([""], 0)
            ClientSMS.init = 1
        else:
            pass
        node = cdp.sms_node()
        tree = cdp.sms_info_pointed(node)
        cdp.sms_numbers(port, 0)
        timeout = 60
        passwd = "1"
        user = get_username()

        if DEBUG:
            print("#MSG: login:", nick, host, port, user)
        handle, rc = cdp.sms_client_login(host, user, passwd,
                                          timeout, port, tree)
        self.handle = handle
        self.host = host
        self.port = port
        self.num = -1
        self.nick = nick
        if nick is None:
            self.nick = "%s@%d" % (self.host, self.port)

    def process_node(self, node, wins):
        if node.type != cdp.NODE_TASK:
            kid = node.kids
            while kid:
                self.process_node(kid, wins)
                kid = kid.next
        else:
            for kind, win in wins.items():
                state = "%s" % ClientSMS.status(node)
                if state != kind:
                    continue
                path = cdp.sms_node_full_name(node)
                win.insert("%s:%s" % (self.nick, path))
                if DEBUG:
                    print(self.host, self.port, path, state)

    def process(self, win):
        rc = cdp.sms_client_news(self.handle, self.num)
        if rc or self.num == -1:
            Label.update()
            state = cdp.sms_client_status2(self.handle, self.num)
            cdp.sms_client_status(self.handle, state, self.num)

            item = cdp.sms_status2node(state)
            if not item:
                return
            self.process_node(item, win)
            Label.update()
            cdp.sms_node_free(item)

    @classmethod
    def status(cls, node):
        s_status = {
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
            9: "unknown", }
        return s_status[node.status]

    def __exit__(self):
        cdp.sms_client_logout(self.handle)
        print("# logout from", self.nick)

    def __delete__(self):
        cdp.sms_client_logout(self.handle)
        print("# logout from", self.nick)



class Application(tki.Frame):
    """ main """

    def __init__(self, parent=None, client=None, queue=None):
        if parent is None: parent = tki.Tk()
        w, h = parent.winfo_screenwidth(), parent.winfo_screenheight()
        tki.Frame.__init__(self, parent)
        if client is None:
            raise Exception("#ERR: please provide a client...\n./overview.py $ECF_HOST@$ECF_PORT")
        elif type(client) in (set, tuple):
            self.__clients = client
        else:
            self.__clients = [client]
        self.__queue = queue
        # width = 640;         height = 780
        width = w;         height = h
        self.canvas = tki.Canvas(width=width, height=height, bg='black')
        self.grid()
        self.createWidgets()
        self.canvas.after(50, self.check_queue)

        self.bind_all("<space>", self.update)
        # self.bind_all("Control_q", self.quit)
        # self.bind_all("Control_z", self.resize)
        self.bind_all("<Control-q>", self.quit)
        self.bind_all("<Control-z>", self.resize)

    def resize(self, event=None): print("New size is: {}x{}".format(event.width, event.height))

    def quit(self, event=None):
        print("quit...")
        self.destroy()
        # global running; running = [ False, ]
        # sys.exit(0)

    def createWidgets(self):
        global root
        root = self
        rowx = 1
        var = tki.StringVar(self)
        glob = Label(root)
        # glob = Label(var)
        self.__menuBar = MenuBar(root)
        self.__menuBar.grid(row=0, column=0, sticky=tki.W)
        self.label = tki.Label(root, textvariable=var) # Label.inst)
        self.label.grid(row=0, column=2, sticky=tki.E)
        self.__wins = dict()
        rowx += 1
        colx = 0
        kinds = ("active", "aborted", "submitted")
        for kind in kinds:
            self.__wins[kind] = TaskList(root, kind)
            self.__wins[kind].grid(row=rowx, column=colx,
                                   sticky=tki.S + tki.E + tki.W)
            colx += 1
        self.update()

    def check_queue(self):
        try:
            self.__queue.get(block=False)
        except Queue.Empty:
            pass
        else:
            self.update()
        self.canvas.after(50, self.check_queue)

    def update(self, event=None):
        self.label.update()
        for kind, win in self.__wins.items():
            win.clear()
        for client in self.__clients:
            if type(client) == list:
                for clt in client:
                    clt.process(self.__wins)
            else:
                client.process(self.__wins)


def get_username(): return pwd.getpwuid(os.getuid())[0]


def get_uid(): return pwd.getpwnam(get_username()).pw_uid


if __name__ == '__main__':
    try:
        port = 1500 + int(get_uid())
    except:
        port = 31415

    if len(sys.argv) > 1:
        clients = []
        for num in xrange(1, len(sys.argv)):            
            sep = ','
            arg = sys.argv[num]
            if sep in arg:
                for one in arg.split(sep):
                    client = Client(one)
            else:
                client = Client(arg)
            clients.append(client)
    else: 
        one = "localhost%d" % port
        print("# using", one)
        clients = [Client(one), ]

    queue = Queue.Queue()
    app = Application(client=clients, queue=queue)
    app.master.title(PROGRAM_NAME)
    app.columnconfigure(0, weight=1)
    app.rowconfigure(0, weight=1)
    PaceKeeper(app, queue)
    app.mainloop()

"""
python ./overview.py localhost@31415,
"""

#!/usr/bin/env python
""" tkinter example with ecflow
"""

import Tkinter as tki
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
        self.__helpButton = self.__createHelp()
        self.__helpButton.grid(row=0, column=3)
        self.__updateButton = tki.Button(self, text='Update', font=BUTTON_FONT,
                                         command=parent.update)
        self.__updateButton.grid(row=0, column=2)

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
        if url is None:
            return
        os.system("firefox " + url)


class TaskList(tki.Frame):
    """ a class to host all tasks"""
    WIDTH = 40
    LINES = 80

    def __init__(self, parent, kind):
        tki.Frame.__init__(self, parent)
        self.__kind = kind
        self.__callback = None
        self.__label = tki.Label(
            self, font=BUTTON_FONT, background=COLORS[kind], text=kind)
        self.__label.grid(row=0, column=0, sticky=tki.W)
        self.__scrolledList = ScrolledList(
            self, width=self.WIDTH, height=self.LINES,
            callback=self.__callback)
        self.__scrolledList.grid(row=1, column=0)

    def insert(self, path):  self.__scrolledList.append(path)

    def clear(self):         self.__scrolledList.clear()


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

    def __init__(self, one="local-localhost@31415"):
        try:
            nick, hhh = one.split("-")
        except:
            hhh = one
            nick = None
        try:
            host, port = hhh.split("@")
        except:
            host = "localhost"
            port = 31415

        if nick is None:
            self.nick = "%s@%d" % (host, port)
        print "# client creation", nick, host, port
        self.nick = nick
        self.client = ec.Client(host, port)

    def process(self, win):
        Label.update()
        self.client.sync_local()
        defs = self.client.get_defs()
        if defs is None:
            print("# %s-%: empty content" % (self.host, self.port))
        Label.update()
        for suite in defs.suites:
            self.process_nc(suite, win)

    def process_nc(self, node, win):
        for item in node.nodes:
            if isinstance(item, ec.Task):
                self.process_node(item, win)
            else:
                self.process_nc(item, win)

    def process_node(self, node, wins):
        for kind, win in wins.items():
            status = "%s" % node.get_state()
            if status != kind:
                continue
            win.insert("%s:%s" % (self.nick, node.get_abs_node_path()))

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
            print "#MSG: login:", nick, host, port, user
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
                    print self.host, self.port, path, state

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
        print "# logout from", self.nick

    def __delete__(self):
        cdp.sms_client_logout(self.handle)
        print "# logout from", self.nick


class Application(tki.Frame):
    """ main """

    def __init__(self, master=None, client=None, queue=None):
        tki.Frame.__init__(self, master)
        if client is None:
            self.__clients = [Client("localhost@31415"), ]
        elif type(client) == set:
            self.__clients = client
        else:
            self.__clients = [client]

        self.__queue = queue
        width = 640
        height = 780
        self.canvas = tki.Canvas(width=width, height=height, bg='black')
        self.grid()
        self.createWidgets()
        self.canvas.after(50, self.check_queue)

    def createWidgets(self):
        rowx = 1
        glob = Label(tki.StringVar(self))
        root = self
        self.__menuBar = MenuBar(root)
        self.__menuBar.grid(row=0, column=0, sticky=tki.W)
        self.label = tki.Label(root, textvariable=Label.inst)
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

    def update(self):
        Label.update()
        for kind, win in self.__wins.items():
            win.clear()
        for client in self.__clients:
            if type(client) == list:
                for clt in client:
                    clt.process(self.__wins)
            else:
                try:
                    client.process(self.__wins)
                except:
                    pass


def get_username(): return pwd.getpwuid(os.getuid())[0]


def get_uid(): return pwd.getpwnam(get_username()).pw_uid


if __name__ == '__main__':
    try:
        port = 1500 + int(get_uid())
    except:
        port = 31415

    if len(sys.argv) > 0:
        clients = []
        for num in xrange(1, len(sys.argv)):
            clients.append(Client(sys.argv[num]))
    else:
        clients = [Client("localhost%d" % port)]

    queue = Queue.Queue()
    # app = AppSms(host, port, queue)
    app = Application(client=clients, queue=queue)
    app.master.title(PROGRAM_NAME)
    app.columnconfigure(0, weight=1)
    app.rowconfigure(0, weight=1)

    PaceKeeper(app, queue)
    app.mainloop()

"""
python ./overview.py localhost-localhost@31415
"""

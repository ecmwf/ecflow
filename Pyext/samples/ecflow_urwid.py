#!/usr/bin/python
"""Thanks https://github.com/pazz/urwidtrees
"""

from __future__ import print_function

import os
import re
import sys
import time

import urwid
import ecflow
mark = 'dirmark'

UPDATE_INTERVAL = 180

singles = {"queued": "Q",
           "submitted": "S",
           "active": "R",
           "complete": "C",
           "aborted": "A",
           "suspended": "P",
           "unknown": "U",
           "RUNNING": "R",
           "HALTED": "H",
           "SHUTDOWN": "S",

           "dirmark": "-",
           # "unkown": "U",
           }
global STATUS_KIND
STATUS_KIND = True


def is_expanded(item):
    return get_status(item, True) in (
        "aborted", "active", "submitted", "RUNNING", "unknown",
        "A", "R", "S", "U", "abo", "RUN", "unk",
    )


def decorate(node, parent=None, keys=None):
    if type(node) not in (ecflow.Defs, ecflow.Alias,
                          ecflow.Suite, ecflow.Family, ecflow.Task):
        if 1:
            raise Exception(type(node))
        return ""
    status = get_status(node)
    ext = ' %c [%s]' % (kinds[type(node)], status)
    num = 0

    if type(node) in (ecflow.Suite, ecflow.Family, ecflow.Task, ):
        if "%s" % node.get_defstatus() == status:
            if status != "queued":
                ext += " (defstatus)"

    if type(node) == ecflow.Defs:
        return ext
    exp = node.get_repeat()
    if exp:
        ext += "\n%s" % exp

    exp = node.get_trigger()
    if exp:
        ext += "\n%s" % exp

    exp = node.get_complete()
    if exp:
        ext += "\n%s" % exp

    exp = node.get_late()
    if exp:
        ext += "\n%s" % exp

    exp = node.get_autocancel()
    if exp:
        ext += "\n%s" % exp

    for item in node.events:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.meters:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.labels:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.limits:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.inlimits:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.times:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.dates:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.todays:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.crons:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.days:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.zombies:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    for item in node.variables:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)
        else:
            ext += " %s" % item
        num += 1

    gvar = ecflow.VariableList()
    node.get_generated_variables(gvar)
    for item in gvar:
        if keys:
            parent._children[num] = LeafNode(item)
            keys.append("%s" % item)

    if keys:
        return num
    return ext


def get_name(item, full=False, parent=None, keys=None):
    if type(item) in (ContainerNode, LeafNode):
        item = item.get_value()
    if type(item) in (dict, tuple, ):
        return "#list"
    if type(item) in (str, ):
        if 1:
            raise Exception(item)
        return item
    if type(item) in (ecflow.Client, ):
        return "%s@%s" % (item.get_host(), item.get_port()) + \
               " - %s " % time.time()

    # ext = ' %c [%s]' % (kinds[type(item)], get_status(item))
    ext = decorate(item)
    if type(item) in (ecflow.Defs, ):
        return "defs" + ext + " - %s " % time.time()

    if type(item) in (ecflow.Client, ):
        return "%s@%s" % (item.get_host(), item.get_port()) + \
               " - %s " % time.time()
    if full:
        return "%s" % item.name() + ext
    return item.name()


def get_status(item, full=False):
    if type(item) in (ecflow.Client, ):
        item = item.get_defs()

    if type(item) in (ecflow.Defs, ):
        out = "%s" % item.get_server_state()
    elif type(item) in (dict, tuple):
        out = "unknown"
    elif type(item) == str:
        if item == dir_sep():
            out = "unknown"
        else:
            raise Exception(item, type(item))
    else:
        out = "%s" % item.get_state()

    if full:
        return out
    if STATUS_KIND == 1:
        return singles[out]
    if STATUS_KIND == 3:
        return out[:3]
    return out


class FlagNodeW(urwid.TreeWidget):
    unexpanded_icon = urwid.AttrMap(urwid.TreeWidget.unexpanded_icon, mark)
    expanded_icon = urwid.AttrMap(urwid.TreeWidget.expanded_icon, mark)

    def __init__(self, item):
        super(FlagNodeW, self).__init__(item)
        self._w = urwid.AttrWrap(self._w, None)
        self.flagged = False
        self.update_w()

    def selectable(self):
        return True

    def keypress(self, size, key):
        key = super(FlagNodeW, self).keypress(size, key)
        if key:
            key = self.unhandled_key(size, key)
        return key

    def unhandled_key(self, size, key):
        if key == " ":
            self.flagged = not self.flagged
            self.update_w()
        else:
            return key

    def update_w(self):
        if self.flagged:
            self._w.attr = 'flagged'
            self._w.focus_attr = 'focus'
        else:
            self._w.attr = 'body'
            self._w.focus_attr = 'focus'
        self._w.attr = get_status(self.get_node().get_value(), True)


class EmptyW(urwid.TreeWidget):

    def get_display_text(self):
        return ('flag', "(empty)")


class ErrorW(urwid.TreeWidget):

    def get_display_text(self):
        return ('flag', "(error)")


class NodeW(FlagNodeW):

    def __init__(self, item):
        super(NodeW, self).__init__(item)
        node = item.get_value()
        add_widget(node, self)

    def get_display_text(self):
        node = self.get_node().get_value()
        return get_name(node, True)


class ContainerNodeW(FlagNodeW):

    def __init__(self, item):
        super(ContainerNodeW, self).__init__(item)
        node = item.get_value()
        add_widget(node, self)
        self.expanded = is_expanded(node)
        self.update_expanded_icon()

    def get_display_text(self):
        node = self.get_node().get_value()
        return get_name(node, True)


class EmptyNode(urwid.TreeNode):

    def load_widget(self):
        return EmptyW(self)


class ErrorNode(urwid.TreeNode):

    def load_widget(self):
        return ErrorW(self)


class LeafNode(urwid.TreeNode):

    def __init__(self, item, parent=None):
        if type(item) == ecflow.Task:
            path = item.get_abs_node_path()
            depth = path.count(dir_sep())
            key = os.path.basename(path)
        else:
            path = "%s" % item
            depth = 0
            key = "%s" % item
        urwid.TreeNode.__init__(self, item, key=key,
                                parent=parent, depth=depth)

    def load_parent(self):
        node = self.get_value()
        if type(node) in (ecflow.Client, ecflow.Defs, ):
            path = dir_sep()
        else:
            path = node.get_abs_node_path()
        parentname, name = os.path.split(path)
        if 1:
            raise Exception
        parent = ContainerNode(self.ci.find_abs_node_path(parentname))
        parent.set_child_node(self.get_key().get_value(), self)
        return parent

    def load_widget(self):
        return NodeW(self)

    # def load_child_keys(self): return [None]


class ContainerNode(urwid.ParentNode):

    def __init__(self, item, parent=None):
        if type(item) in (ecflow.Client, ecflow.Defs, ):
            path = dir_sep()
        else:
            path = item.get_abs_node_path()
        depth = path.count(dir_sep())
        if path == dir_sep():
            depth = 0
            key = None
        else:
            key = os.path.basename(path)
        urwid.ParentNode.__init__(
            self, item, key=key, parent=parent, depth=depth)

    def load_parent(self):
        node = self.get_value()
        if type(node) in (ecflow.Client, ecflow.Defs, ):
            path = dir_sep()
        else:
            path = node.get_abs_node_path()
        parentname, name = os.path.split(path)
        if 1:
            raise Exception
        parent = ContainerNode(self.ci.find_abs_node_path(parentname))
        parent.set_child_node(self.get_key().get_value(), self)
        return parent

    def load_child_keys(self):
        keys = []
        num = 0
        node = self.get_value()
        if type(node) in (ecflow.Client, ):
            node = node.get_defs()

        if type(node) in (ecflow.Defs, ):
            for item in node.suites:
                self._children[num] = ContainerNode(item)
                keys.append(get_name(item))
                num += 1

        elif type(node) in (ecflow.Suite, ecflow.Family, ):
            for item in node.nodes:
                if type(item) in (ecflow.Family, ):
                    self._children[num] = ContainerNode(item)
                elif type(item) in (ecflow.Task, ):
                    self._children[num] = LeafNode(item)
                else:
                    self._children[num] = LeafNode(item)
                keys.append(get_name(item))
                num += 1

            if 0:
                num += decorate(node, self, keys)

        elif type(node) in (ecflow.Task, ):
            pass

        elif 1:
            raise Exception(type(node))
        else:
            self._children[0] = EmptyNode(
                self, parent=self, key=None, depth=num)
        self.dir_count = num + 1
        # print(keys)
        return keys

    def load_child_node(self, key):
        # index = self.get_child_index(key)
        node = self.get_value()
        num = 0
        if type(node) in (ecflow.Client, ):
            node = node.get_defs()

        if type(node) in (ecflow.Task, ):
            return EmptyNode(None)

        if type(node) in (ecflow.Suite, ecflow.Family):
            num = 0
            for item in node.nodes:
                if num == key:
                    node = item
                    break
                if item.name() == key:
                    node = item
                    break
                num += 1

        if key is None:
            return EmptyNode(None)

        if type(node) in (ecflow.Defs, ):
            path = dir_sep()
            for item in node.suites:
                if num == key:
                    node = item
                    break
                if get_name(item) == key:
                    node = item
                    break
                num += 1
            return ContainerNode(node, parent=self)

        # path = os.path.join(get_name(node), key)
        if type(node) in (ecflow.Suite, ecflow.Family, ):
            return ContainerNode(node, parent=self)

        return LeafNode(node, parent=self)

    def load_widget(self):
        return ContainerNodeW(self)


class EcflowBrowser(object):
    palette = [
        ('body', 'black', 'light gray'),
        ('flagged', 'black', 'dark green', ('bold', 'underline')),
        ('focus', 'light gray', 'dark blue', 'standout'),
        ('flagged focus', 'yellow', 'dark cyan',
         ('bold', 'standout', 'underline')),
        ('head', 'yellow', 'black', 'standout'),
        ('foot', 'light gray', 'black'),
        ('key', 'light cyan', 'black', 'underline'),
        ('title', 'white', 'black', 'bold'),
        ('dirmark', 'black', 'dark cyan', 'bold'),
        ('flag', 'dark gray', 'light gray'),
        ('error', 'dark red', 'light gray'),

        ('queued', 'dark gray', 'light blue'),
        ('submitted', 'dark gray', 'light cyan'),
        ('active', 'dark gray', 'light green'),
        ('complete', 'dark gray', 'yellow'),
        ('aborted', 'dark gray', 'dark red'),
        ('unknown', 'dark gray', 'light gray'),
        ('suspended', 'dark gray', 'brown'),
        ('RUNNING', 'dark gray', 'light green'),
        ('HALTED', 'dark gray', 'dark cyan'),  # purple
        ('SHUTDOWN', 'dark gray', 'light red'),  # pink'),
    ]

    footer_text = [
        ('title', "Directory Browser"), "    ",
        ('key', "UP"), ",", ('key', "DOWN"), ",",
        ('key', "PAGE UP"), ",", ('key', "PAGE DOWN"),
        "  ",
        ('key', "SPACE"), "  ",
        ('key', "+"), ",",
        ('key', "-"), "  ",
        ('key', "LEFT"), "  ",
        ('key', "HOME"), "  ",
        ('key', "END"), "  ",
        ('key', "Q"),
    ]

    def __init__(self, server, suites):
        host, port = server.split('@')
        self.ci = ecflow.Client(host, int(port))
        self.ci.ch_register(False, suites)
        self.ci.sync_local()
        # print(self.ci.get_defs())
        self.start_time = time.time()

        # cwd = os.getcwd()
        cwd = self.ci.get_defs()
        store_initial_cwd(dir_sep())
        self.header = urwid.Text("")
        self.listbox = urwid.TreeListBox(urwid.TreeWalker(ContainerNode(cwd)))
        self.listbox.offset_rows = 1
        self.footer = urwid.AttrWrap(urwid.Text(self.footer_text),
                                     'foot')
        self.view = urwid.Frame(
            urwid.AttrWrap(self.listbox, 'body'),
            header=urwid.AttrWrap(self.header, 'head'),
            footer=self.footer)

    def main(self):
        """Run the program."""

        self.loop = urwid.MainLoop(self.view, self.palette,
                                   unhandled_input=self.unhandled_input)
        self.loop.run()

        # on exit, write the flagged filenames to the console
        names = [escape_filename_sh(x) for x in get_flagged_names()]
        print(" ".join(names))

    def unhandled_input(self, k):
        # update display of focus directory
        if k in ('q', 'Q'):
            raise urwid.ExitMainLoop()

        if k in ('z', 'Z', ):
            self.ci.sync_local()

        global STATUS_KIND
        if k in ('3', "3", 3):
            STATUS_KIND = 3

        elif k in ('1', "1", 1):
            STATUS_KIND = 1

        elif k in ('f', "F"):  # full
            STATUS_KIND = 0


def main(server, suites):
    EcflowBrowser(server, suites).main()


_widget_cache = {}


def add_widget(path, widget):
    _widget_cache[path] = widget


def get_flagged_names():
    lst = []
    for w in _widget_cache.values():
        if w.flagged:
            node = w.get_node().get_value()
            lst.append(get_name(node))
    return lst


_initial_cwd = []


def store_initial_cwd(name):
    global _initial_cwd
    _initial_cwd = name.split(dir_sep())


def escape_filename_sh(name):
    """Return a hopefully safe shell-escaped version of a filename."""

    # check whether we have unprintable characters
    for ch in name:
        if ord(ch) < 32:
            # found one so use the ansi-c escaping
            return escape_filename_sh_ansic(name)

    # all printable characters, so return a double-quoted version
    name.replace('\\', '\\\\')
    name.replace('"', '\\"')
    name.replace('`', '\\`')
    name.replace('$', '\\$')
    return '"' + name + '"'


def escape_filename_sh_ansic(name):
    """Return an ansi-c shell-escaped version of a filename."""

    out = []
    # gather the escaped characters into a list
    for ch in name:
        if ord(ch) < 32:
            out.append("\\x%02x" % ord(ch))
        elif ch == '\\':
            out.append('\\\\')
        else:
            out.append(ch)

    # slap them back together in an ansi-c quote  $'...'
    return "$'" + "".join(out) + "'"


SPLIT_RE = re.compile(r'[a-zA-Z]+|\d+')


def alphabetize(s):
    L = []
    import itertools
    for isdigit, group in itertools.groupby(
            SPLIT_RE.findall(s), key=lambda x: x.isdigit()):
        if isdigit:
            for n in group:
                L.append(('', int(n)))
        else:
            L.append((''.join(group).lower(), 0))
    return L


def dir_sep():
    return getattr(os.path, 'se[', '/')


kinds = {ecflow.Defs: "D",
         ecflow.Client: "C",
         ecflow.Suite: "S",
         ecflow.Family: "F",
         ecflow.Task: "T",
         ContainerNode: "X",
         LeafNode: "L",
         str: "S",
         }


if __name__ == '__main__':
    if len(sys.argv) == 2:
        node = sys.argv[1]
    else:
        raise Exception("ecflow_urwid.py localhost@2500/test")
    server, suites = node.split('/')
    main(server, [suites])

"""
python ecflow_urwid.py localhost@2500/test

"""

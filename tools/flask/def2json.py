import re
import json
import sys
sys.path.append('/usr/local/apps/ecflow/current/lib/python2.7/site-packages/')
import ecflow as ec

pattern = "^[ ]*(?P<kind>[\w])[ \t]*(?P<content>[\w]+)?$"
pattern = "\s*(?P<kind>[\w]+)\s*(?P<content>[.]*)"
check = re.compile(pattern)
inc = "  "


class ListVarUse(object):

    def __init__(self, defs, name):
        self.memo = []
        self.name = str(name)
        self.defs = defs

    def add(self, node):
        if node is None:
            return

        res = [node.get_abs_node_path() +
               ":" + var.name() + " " + var.value()
               for var in node.variables
               if str(var.name()) == self.name]

        if not res:
            gvar = ec.VariableList()
            node.get_generated_variables(gvar)
            res = [node.get_abs_node_path() +
                   ":" + var.name() + " " + var.value()
                   for var in gvar
                   if var.name() == self.name]

        if res:
            self.memo += res

    def process(self, path=None, item=None):
        if path is not None:
            xxx = path
            if path[-1] == '/':
                xxx = path[:-1]
            item = self.defs.find_abs_node(str(xxx))
            self.add(item)
            self.process(item=item)
            print xxx, path, self.memo
            return self.report()

        elif item is None:
            for node in self.defs.suites:
                self.add(node)
                self.process(item=node)

        else:
            if isinstance(item, ec.Task) or isinstance(item, ec.Alias):
                return

            for node in item.nodes:
                self.add(node)
                self.process(item=node)

    def report(self):
        return self.memo


def add_trig(expr, kind, addto):
    if not expr:
        return
    for part_expr in expr.parts:
        trig = kind
        if part_expr.and_expr():
            trig += "-a "
        if part_expr.or_expr():
            trig += "-o "
        addto.__dict__[kind] = "%s" % trig + " %s" % part_expr.get_expression()


def cleaner(dct):
    if not isinstance(dct, dict):
        return dct
    return dict((k, cleaner(v))
                for k, v in dct.iteritems()
                if v is not None)


ATTRIBUTES = (("autocancel", "a"),
              # automigrate  autorestore
              ("clock", "a"),
              ("complete", "T"),
              ("cron", "t"),
              ("date", "t"),
              ("day", "t"),
              ("defstatus", "a"),
              ("edit", "V"),
              ("event", "e"),
              ("inlimit", "l"),
              ("label", "a"),
              ("late", "a"),
              ("limit", "l"),
              ("meter", "e"),
              # owner
              ("repeat", "R"),
              # text
              ("time", "t"),
              ("today", "t"),
              ("trigger", "T"),
              )


def att_filter(filt, kind):
    if filt == "all":
        return True
    for key, val in ATTRIBUTES:
        if key == kind and val in filt:
            return True
    return False


class Tree(object):

    def __init__(self, node, attr=1):
        self.__node = node
        if not self.is_task():
            self._kids = [Tree(kid) for kid in self.kids()]
        self._status = "%s" % node.get_state()
        if self.is_def():
            pass  # FIXME
        elif attr:
            self.add_attributes(node)
        cleaner(self.__dict__)

    def add_attributes(self, node, filt="all"):
        # add_trig(node.get_complete(), "complete", self)
        # add_trig(node.get_trigger(), "trigger", self)
        out = []
        if self.is_def():
            return
        if self.is_suite() and node.get_clock():
            if att_filter(filt, "clock"):
                out.append("%s" % node.get_clock())
            else:
                pass
        else:
            pass

        for item in node.crons:
            if item and att_filter(filt, "cron"):
                out.append("%s" % item)
        for item in node.dates:
            if item and att_filter(filt, "date"):
                out.append("%s" % item)
        for item in node.days:
            if item and att_filter(filt, "day"):
                out.append("%s" % item)
        for item in node.times:
            if item and att_filter(filt, "time"):
                out.append("%s" % item)
        for item in node.todays:
            if item and att_filter(filt, "today"):
                out.append("%s" % item)
        for item in node.inlimits:
            if item and att_filter(filt, "inlimit"):
                out.append("%s" % item)
        for item in node.limits:
            if item and att_filter(filt, "limit"):
                out.append("%s" % item)
        for item in node.events:
            if item and att_filter(filt, "event"):
                out.append("%s" % item)
        for item in node.meters:
            if item and att_filter(filt, "meter"):
                out.append("%s" % item)
        for item in node.labels:
            if item and att_filter(filt, "label"):
                out.append("%s" % item)
        for item in node.variables:
            if item and att_filter(filt, "edit"):
                out.append("%s" % item)

        for kind, item in (
            ("autocancel", node.get_autocancel()),

            ("complete", node.get_complete()),
            ("trigger", node.get_trigger()),

            ("late", node.get_late()),
        ):
            if item and att_filter(filt, kind):
                out.append("%s" % item)
            else:
                pass

        if isinstance(node, Tree):
            pass
        elif self.is_def():
            return

        defstatus = "%s" % node.get_defstatus()
        if "queued" not in defstatus and att_filter(filt, "defstatus"):
            out.append(defstatus)

        repeat = node.get_repeat()
        if repeat and att_filter(filt, "repeat"):
            if not repeat.empty():
                self._repeat = "%s" % repeat

        # generated variables
        if att_filter(filt, "edit"):
            gvar = ec.VariableList()
            node.get_generated_variables(gvar)
            for var in gvar:
                out.append("%s" % var)

    def is_family(self):
        return isinstance(self.__node, ec.ecflow.Family)

    def is_task(self):
        return isinstance(self.__node, ec.ecflow.Task)

    def is_suite(self):
        return isinstance(self.__node, ec.ecflow.Suite)

    def is_def(self):
        return isinstance(self.__node, ec.ecflow.Defs)

    def kids(self):
        if self.is_def():
            return [node
                    for node in self.__node.suites]

        elif self.is_family() or self.is_suite():
            return [node
                    for node in self.__node.nodes]

        else:
            return

    def _name(self):
        if self.is_def():
            return '/'
        elif isinstance(self.__node, Tree):
            return self.__node._name
        else:
            return self.__node.name()

    def _fullname(self):
        if self.is_def():
            return '/'
        else:
            return self.__node.get_abs_node_path()

    # def __unicode__(self): return unicode(self.some_field) or u''

    def __str__(self):
        """ translate into string """
        out = "tree " + "%s" % self.__node
        if not self.is_task():
            for item in self.kids():
                out += "%s" % item
        return out

    def __iter__(self):
        if self.is_def():
            for item in self.__node.suites:
                yield item
        else:
            for item in self.__node.nodes:
                yield item

    def _kind(self):
        if self.is_task():
            return "task"
        elif self.is_family():
            return "family"
        elif self.is_suite():
            return "suite"
        elif self.is_def():
            return "definition"
        elif isinstance(self.__node, Tree):
            return "tree"
        else:
            return "%" % type(self.__node)

    def to_json(self):
        return json.dumps(self,
                          default=lambda o: o.__dict__,
                          ensure_ascii=True,
                          skipkeys=True,
                          sort_keys=True,
                          indent=2)

    def __repr__(self):
        return "%s" % json.dumps(
            self.__dict__,
            encoding="utf-8",
            skipkeys=True, sort_keys=True, indent=2)


STATUSES = {"U": "unknown",
            "Q": "queued",
            "S": "submitted",
            "R": "active",
            "A": "aborted",
            "C": "complete",
            "P": "suspended",
            "H": "halted",
            "D": "shutdown",
            }


def tostatuses(filt):
    if not filt:
        return
    if "op" in filt:
        filt = "RASP"  # RUNNING-ABORTED-SUBMITTED
    if "adm" in filt:
        filt = "RASPQCU"  # aka all

    res = []
    for key, val in STATUSES.items():
        if filt is None:
            res.append(val)
        elif key in filt:
            res.append(val)
        elif filt in ("all", ):
            res.append(val)

    return res


class TreeD3JS(Tree):
    """ filter status, filter_attribute"""

    def __init__(self, node, fis=None, fia=None, depth=10):
        super(TreeD3JS, self).__init__(node, attr=0)
        self.name = self._name()
        if "_kids" in self.__dict__.keys():
            del self._kids
        self.size = 1
        self.kind = self._kind()

        if fia:
            if fia != "none":
                self.attr = self.add_attributes(node)

        if self.is_task():
            return
        if int(depth) < 0:
            return
        if len(self.kids()) > 0:
            statuses = tostatuses(fis)
            if not statuses:
                return
            self.children = []
            for kid in self.kids():
                if "%s" % kid.get_state() in statuses:
                    self.children.append(
                        TreeD3JS(kid, fis, fia, int(depth) - 1))
                else:
                    pass
                self.size += 100 * len(self.children)


if __name__ == "__main__":
    pass

"""
"""

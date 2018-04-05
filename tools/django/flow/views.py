# coding: utf-8
"""
source ./venv/bin/activate; dest=$HOME/http/; cd $dest/fdi
nohup python manage.py runserver 0.0.0.0:8001 > /tmp/django.tpm 2>&1 &
"""
from __future__ import print_function    # (at top of module)
from django.http import HttpResponse
from django.conf import settings
from django import forms  # newforms as forms
import django.template as dt
from django.shortcuts import render_to_response
from django.template.context_processors import csrf
from django.views.decorators.csrf import csrf_protect
from django.shortcuts import *
from flow.settings import MEDIA_ROOT
import os
import sys
import time
from os.path import join, getsize
# from templatetags import sync_pull
import models
import contextlib
from threading import Timer
from django.contrib.auth import authenticate, login
from django.contrib.auth.decorators import login_required
import clt
import platform
try:
    from StringIO import StringIO
except:
    from io import StringIO

PORT = int(os.getenv("ECFLOW_DJANGO_PORT", "8001"))
HOST = platform.node() + ".ecmwf.int:%d" % PORT
# HOST = "localhost:%d" % PORT

from django.utils.encoding import force_unicode
from django.utils.html import escape, mark_safe
from cgi import parse_qs, escape
from urlparse import urlparse
from django.core.urlresolvers import reverse, resolve

import django.core.handlers.wsgi
import pprint
import json
import ecflow as ec
from subprocess import Popen, PIPE

import traceback

from . import def2json
from . import ecf
# from gevent import monkey, Greenlet; monkey.patch_all()
# from gevent.queue import Queue
# from gevent.subprocess import Popen, PIPE
# from flask_redis import Redis
# redis_store = Redis()

COMPRESS = True
COMPRESS = False
DEBUG = False
# attribute, time, event-like, limit-related, Repeat, Variable, Trigger
ATTRIBUTES = (("autocancel", "a"),
              # automigrate  autorestore
              ("clock", "a"),

              ("complete", "T"),
              ("trigger", "T"),

              ("cron", "t"),
              ("date", "t"),
              ("day", "t"),
              ("defstatus", "a"),
              ("edit", "V"),
              ("edit", "G"),
              ("event", "e"),
              ("inlimit", "l"),
              ("label", "a"),
              ("late", "a"),
              ("limit", "l"),
              ("meter", "e"),
              # owner
              ("repeat", "R"),
              #         text
              ("time", "t"),
              ("today", "t"),
              )

SERVER_CMD = ("suites", "stats", "ping", "history", "zombie",
              "script", "man", "job", "output", "timeline",
              # "info", "msg", "var", "why",
              )

cdp = '/usr/local/apps/sms/bin/cdp'


def slide2(request):
    template = dt.loader.get_template('ecflow2.html')
    return HttpResponse(template.render(dict()))
    # return render_to_response('template/ecflow2.html')


def slide3(request):
    template = dt.loader.get_template('ecflow3.html')
    return HttpResponse(template.render(dict()))


class ParanoidKeyError(Exception):
    """
    This can't be a KeyError subclass or Django template
    rendering will swallow it.
    """


class ParanoidContextProxy(object):
    """
    This is a poor-man's proxy for a context instance.

    Make sure template rendering stops immediately on a KeyError.
    """

    def __init__(self, context):
        self.context = context
        self.seen_keys = set()

    def __getitem__(self, key):
        self.seen_keys.add(key)
        try:
            return self.context[key]
        except KeyError:
            raise ParanoidKeyError('ParanoidKeyError: %r' % (key,))

    def __getattr__(self, name):
        return getattr(self.context, name)

    def __setitem__(self, key, value):
        self.context[key] = value

    def __delitem__(self, key):
        del self.context[key]


@contextlib.contextmanager
def paranoid_context_manager(context, expected_keys=None):
    """
    use ParanoidContextProxy *and* make sure keys in expected_keys iterable
    are all referenced inside the with statement.
    """
    yield ParanoidContextProxy(context)

    if expected_keys:
        unref = set(expected_keys).difference(context.seen_keys)
        if unref:
            raise ParanoidKeyError('Keys not referenced: %r' % (unref,))


def paranoid_render_to_response(template_name, dictionary=None,
                                context_instance=None, *args, **kwargs):
    """
    use ParanoidContextProxy *and* require that all variables passed
    in the data dictionary are used by templates at least once.
    """
    if dictionary is None:
        dictionary = {}
    with paranoid_context_manager(context_instance, dictionary.keys()) as c:
        return render_to_response(template_name, dictionary, c, *args, **kwargs)


def home(request):
    template = dt.loader.get_template('main.html')
    return HttpResponse(template.render({'HOST': HOST,
                                         'action': "", }))


def treemap(request):
    template = dt.loader.get_template('treemap.html')
    context = dt.Context({})
    return HttpResponse(template.render(context))


def sunburst(request):
    template = dt.loader.get_template('sunburst.html')
    return HttpResponse(template.render(dt.Context({"HOST": HOST})))


def contact(client, sep=':'):
    res = 'localhost%c31415' % sep
    if isinstance(client, dict):
        if "loalhost" == str(client['host']):
            client['host'] = 'localhost'
        res = "%s%c%s" % (client['host'], sep, client['port'])
    elif isinstance(client, ec.Client):
        res = "%s%c%s" % (client.get_host(), sep, client.get_port())
    return res


form_template = """<!DOCTYPE html>
<html>
<head>
<meta http-equiv=”refresh” content=”60" >
<meta charset="utf-8">
        <!-- jQuery should be at least version 1.7 -->
        <script src="http://code.jquery.com/jquery-1.11.0.min.js"></script>
        <script src="/static/assets/contextmenu.js"></script> 
        <link rel="stylesheet" type="text/css" href="/static/assets/contextmenu.css" />

<link rel="stylesheet" type="text/css" href="/static/assets/sms.css" />
<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.11.2/jquery.min.js"></script>
<script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.2/js/bootstrap.min.js"></script>
<script type="text/javascript" src="/static/assets/sms.js"></script>
</head>

<body>
<div class="top">"""


def get_username():
    import pwd
    import os
    return pwd.getpwuid(os.getuid())[0]


def get_uid():
    import pwd
    return pwd.getpwnam(get_username()).pw_uid


def ecf_port():
    return 1500 + get_uid()


client = os.getenv('ECF_HOST', 'localhost') + \
    '/%s' % os.getenv('ECF_PORT', str(ecf_port()))

form_template += """</div>
<p>{{ body | safe}}

<div id="test1">
<a href="http://%s/api/ecflow/servers/treemap" class="test">ecFlow servers</a>
<a href="http://%s/api/sms/servers/treemap" class="test">SMS servers</a><br/>
<a href="http://%s/api/ecflow/localhost/%d/suite/sunburst/all/none/5/_all_"
   class="test">sunburst</a>
<a href="http://%s/api/ecflow/%s/suite/radial/all/none/5/_all_"
   class="test">radial</a><br/>
<a href="http://%s/api/ecflow/%s/suite/sunburs2/SAR/none/5/test"
   class="test">e-suite</a>
<a href="http://%s/api/ecflow/%s/suite/radial/SAR/none/5/test"
   class="test">test</a>
<a href="http://%s/api/ecflow/%s/suite/treemap/SARQCU/non/5/test"
   class="test">test</a>
        </div>
        <!-- initially hidden right-click menu -->
        <!--div class="hide" id="rmenu">
<ul><li><a href="http://%s/login">Localhost</a></li></ul></div-->
</body>
</html>""" % (HOST, HOST,
              HOST, PORT,
              HOST, client,
              HOST, client,
              HOST, client,
              HOST, client, HOST,
              )


class GenericScheduler(object):

    def __init__(self, name="generic", servers=None):
        self.rest = "http://%s/api/%s/" % (HOST, name)
        self.servers_list = servers
        self.serv = dict()
        self.name = name
        self.req = ""
        self.clients = dict()
        self.load_list()

    def load_list(self):
        if not self.servers_list:
            return
        for name in self.servers_list:
            if not os.path.isfile(name):
                continue
            with open(name, 'r') as servers_file:
                for line in servers_file:
                    try:
                        if "," in line:
                            nick, host, port = line.split(",")
                        else:
                            nick, host, port = line.split()
                        self.serv[nick] = {'host': host,
                                           'port': port, }
                    except:
                        pass

    def resturl(self, add=""):
        return self.rest + add

    def servers(self):
        return self.serv

    def store(self, req):
        if isinstance(req, django.core.handlers.wsgi.WSGIRequest):
            req = req.path_info
        self.req = req

    def client(self, key):
        return self.clients[key]

    def suites(self, key):
        return "o"

    def ping(self, key):
        return "ping"


CURRENT = "/usr/local/apps/ecflow/current/"
CLIENT = "ecflow_client"
CLIENT = CURRENT + "bin/ecflow_client"


class EcflowScheduler(GenericScheduler):

    def __init__(self):
        super(EcflowScheduler, self).__init__(
            "ecflow",
            [CURRENT + "share/ecflow/servers",
             "/usr/local/share/ecflow/servers",
             os.getenv("HOME") + "/.ecflow_ui/servers.txt",
             os.getenv("HOME") + "/.ecflowrc/servers,", ]
        )
        self.prev = None

    def client(self, key):
        # print(key)
        if len(self.clients) == 0:
            self.clients[key] = ec.Client(key)
        elif key not in self.clients.keys():
            self.clients[key] = ec.Client(key)
        return self.clients[key]

    def suites(self, key):
        return self.client(contact(key)).suites()

    def ping(self, key):
        host, prog = key.split(":")
        proc = Popen([CLIENT, "--port", prog, "--host", host, "--ping"],
                     stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate("")
        if 'Failed' in err:
            return 'ping_nok ' + key
        elif ' succeeded' in out:
            return 'ping_ok'
        return 'ping_unk2 - %s - %s' % (out, err)

    def login_get_show(self, key, name):
        clt = self.client(key)
        if name != "_all_":
            clt.ch_register(False, [name, ])
        res = name
        clt.sync_local()
        defs = clt.get_defs()
        if defs is None:
            res = "empty server %s" % contact(clt)
        for item in defs.suites:
            res += item.name() + "<br />"
        old_stdout = sys.stdout
        result = StringIO()
        sys.stdout = result
        ec.PrintStyle.set_style(ec.Style.STATE)
        # print(defs)
        sys.stdout = old_stdout
        # content = "%s" % defs
        content = result.getvalue()
        name += "<br/>" + content.replace("\n", "<br />")
        clt.ch_drop()
        return name

    def find(self, key, url):
        if injection(url):
            return 'nop'
        name = 'ECF_HOME'
        suite = '/'
        path = '/'
        try:
            surl = url.split('/')
            name = str(surl[7])
            suite = str(surl[8])
            path = str('/%s/' % suite + '/'.join(surl[9:]))
        except:
            print('#ERR:', url, [x for x in enumerate(url.split('/'))])

        print('#WAR:', url, [x for x in enumerate(url.split('/'))])

        clt = self.client(key)
        if not name in('/', '_all_', '', None):
            clt.ch_register(False, [suite, ])

        clt.sync_local()
        defs = clt.get_defs()
        clt.ch_drop()

        proc = def2json.ListVarUse(defs, name)
        return proc.process(path)

    def json(self, var):  # ECFLOW
        clt = self.client(contact(var))

        items = (('suite', 'jonas'),
                 ('fis', 'oper'),
                 ('fia', 'all'),
                 ('depth', 5))
        for key, val in items:
            if key not in var.keys():
                var[key] = val

        res = "json"
        tree = None
        if 1:  # try:
            name = "_all_"
            if 'suite' in var.keys():
                name = var["suite"]
            if name != "_all_":
                clt.ch_register(False, [name, ])
            now = time.ctime()
            if self.prev:
                print(self.prev, now)
            if not self.prev:
                clt.sync_local()
                defs = clt.get_defs()
                self.defs = defs
                self.prev = now
            elif 0:  # now - self.prev > 300:
                clt.sync_local()
                defs = clt.get_defs()
                self.defs = defs
                self.prev = now
            else:
                defs = self.defs

            if defs is None:
                res = "empty server %s" % contact(clt)
            if name != "_all_":
                clt.ch_drop()

            if var["fis"] in ("ops", "oper"):
                var["fis"] = "SAR"
            elif "adm" in var["fis"]:
                var["fis"] = "SARQCUP"
            try:
                depth = int(var['depth'])
            except:
                depth = 5
                print("#ERR: depth", var['depth'])
            tree = def2json.TreeD3JS(node=defs,
                                      fis=var['fis'],
                                      fia=var['fia'],
                                      depth=depth)
            jsontree = tree.to_json()
            # jsontree = ecf.to_d3js(defs)
            # print(# defs,                  tree, jsontree)
            if jsontree is None:
                jsontree = 'none'
            # out = '[%s]' % jsontree
            out = '%s' % jsontree

            if COMPRESS:
                import gzip
                import cStringIO
                zbuf = cStringIO.StringIO()
                zfile = gzip.GzipFile(mode='wb', compresslevel=0,
                                      fileobj=zbuf)
                zfile.write(out.encode('utf-8'))
                zfile.close()
                content = zbuf.getvalue()
                res = HttpResponse(content)
                res['Content-Encoding'] = 'gzip'
                res['Content-Length'] = str(len(content))
                return res
            return HttpResponse(out,
                                content_type='application/json')

        elif 0:  # except BaseException, e:
            # thanks
            # http://stackoverflow.com/questions/1278705/python-when-i-catch-an-exception-how-do-i-get-the-type-file-and-line-number
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            res = '%s %s %s %s' % (e.args, exc_type, exc_tb.tb_lineno, fname)
        return HttpResponse(res)


def injection(strs):
    for item in strs:
        if '$' in item or ';' in item:
            return True
    return False


class SMSScheduler(GenericScheduler):

    def __init__(self):
        super(SMSScheduler, self).__init__(
            'sms',
            '/usr/local/apps/sms/lib/servers')
        self.handle = None

    def client(self, key):
        # if 1:            return None
        if def2json.NOCDP:
            return None
        if key not in self.clients.keys():
            self.clients[key] = def2json.SMSClient(key)
        # if DEBUG:            print('key:', key)
        return self.clients[key]

    def ping(self, key):
        host, prog = key.split(':')
        os.putenv('SMS_PROG', prog)
        proc = Popen(['smsping', host], stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate('')
        if 'errorcode' in out:
            return 'ping_nok ' + key
        elif ' OK!' in out:
            return 'ping_ok'
        return 'ping_unk2 - %s - %s' % (out, err)

    def suites(self, key):
        host, port = contact(key).split(':')
        self.host, self.port = [host, port]
        user = get_username()
        # cdp = '/usr/local/apps/sms/bin/cdp'
        if injection((port, host, user)):
            return 'dont do that'
        arg = 'set SMS_PROG %s; login %s %s 1; suites' % (
            port, host, user)
        proc = Popen([cdp, '-q', '-c', arg, ';\n'],
                     stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate('')
        res = ''
        ignore = True
        for line in out.split('\n'):
            if 'Suites you' in line:
                ignore = True
                break
            elif not ignore:
                res += line
            elif 'Suites defined' in line:
                ignore = False
        # res += ' _all_'
        return [var for var in res.split(' ') if var]

    def login_get_show(self, key, name):
        host, port = key.split(':')
        user = get_username()
        if injection((port, host, user, name)):
            return 'dont do that'

        arg = 'set SMS_PROG %s; login %s %s 1; get /%s; show /%s' % (
            port, host, user, name, name)
        proc = Popen([cdp, '-q', '-c', arg, ';\n'],
                     stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate('')
        return out.replace('\n', '<br />')

    def find(self, key, url):
        return 'Not yet!'

    def times(self, key, url):
        return 'Not yet!'

    def json(self, var):  # SMS
        if True:
            return HttpResponse({})
        clt = self.client(contact(var))

        items = (('suite', 'jonas'),
                 ('fis', 'oper'),
                 ('fia', 'all'),
                 ('depth', 5))
        for key, val in items:
            if key not in var.keys():
                var[key] = val

        res = 'json'
        tree = None
        if 1:  # try:
            name = '_all_'
            if 'suite' in var.keys():
                name = var['suite']
            if var['port'] == 314159:
                node = 'localhost'
            else:
                node = var['host']
            clt = def2json.SMSClient('%s:%d' % (node, var['port']))
            defs = clt.update()

            # BUG catch kill signal load_signals();client.c
            jsontree = None
            if defs is None:
                res = 'empty server %s' % contact(clt)
            if 0:
                tree = def2json.TreeD3JS_SMS(defs,
                                             fis=var['fis'],
                                             fia=var['fia'],
                                             depth=int(var['depth']))
                jsontree = tree.to_json()  # KILL??? FIXME???
            else:
                cmd = 'python ./def2json.py -h %s -p %d -s -f %s -r %s -o sms.json' % (
                    node,
                    var['port'],
                    var['fis'],
                    var['path'])
                # print("#MSG:", cmd)
                proc = Popen(cmd.split(' '))
                out, err = proc.communicate('')
                with open('sms.json') as jfile:
                    dct = def2json.not_unicode(json.load(
                        jfile,
                        object_hook=def2json.not_unicode),
                        ignore=1)

                    # jsontree= pprint.pformat(dct, indent=2)

                    jsontree = json.dumps(dct,
                                          default=def2json.jdefault,
                                          ensure_ascii=True,
                                          skipkeys=True,
                                          sort_keys=True,
                                          indent=2)

            if jsontree is None:
                jsontree = 'none'
            # out = '[%s]' % jsontree
            out = '%s' % jsontree

            if COMPRESS:
                import gzip
                import cStringIO
                zbuf = cStringIO.StringIO()
                zfile = gzip.GzipFile(mode='wb', compresslevel=0,
                                      fileobj=zbuf)
                zfile.write(out.encode('utf-8'))
                zfile.close()
                content = zbuf.getvalue()
                res = HttpResponse(content)
                res['Content-Encoding'] = 'gzip'
                res['Content-Length'] = str(len(content))
                return res
            return HttpResponse(out,
                                content_type='application/json')

        else:  # except BaseException, e:
            # thanks
            # http://stackoverflow.com/questions/1278705/python-when-i-catch-an-exception-how-do-i-get-the-type-file-and-line-number
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            res = '%s %s %s %s' % (e.args, exc_type, exc_tb.tb_lineno, fname)
            print('#exception!!!')
        return HttpResponse(res)


class Scheduler(GenericScheduler):

    def __init__(self):
        self.schedulers = (EcflowScheduler(),
                           SMSScheduler(), )
        super(Scheduler, self).__init__(name='meta')

    def load_list(self):
        for item in self.schedulers:
            item.load_list()

    def resturl(self, add=''):
        for item in self.schedulers:
            if item.name in self.req:
                return item.rest + add
        return self.rest

    def servers(self):
        for item in self.schedulers:
            if item.name in self.req:
                return item.serv
        return self.serv

    # FIXME there is pattern proxy
    def client(self, key):
        for item in self.schedulers:
            if item.name in self.req:
                return item.client(contact(key))
        return None

    def suites(self, key):
        for item in self.schedulers:
            if item.name in self.req:
                return item.suites(key)
        return None

    def ping(self, key):
        for item in self.schedulers:
            if item.name in self.req:
                return item.ping(key)
        return 'ping_unk3'

    def login_get_show(self, key, name):
        for item in self.schedulers:
            if item.name in self.req:
                return item.login_get_show(key, name)
        return None

    def find(self, key, name):
        for item in self.schedulers:
            if item.name in self.req:
                return item.find(key, name)
        return 'none'

    def times(self, key, name):
        for item in self.schedulers:
            if item.name in self.req:
                return item.times(key, name)
        return dict()

    def json(self, var):
        for item in self.schedulers:
            if item.name in self.req:
                if 'ecflow' in self.req and '433334' in self.req:
                    continue
                return item.json(var)
        return None


class ProcessURL(object):

    def __init__(self, url):
        self.url = url.split('/')
        self.var = {'host': 'localhost',
                    'port': '31415',
                    'kind': 'servers',
                    # 'kind': 'suite',
                    'suite': '_all_',
                    'path': '/',
                    'display': 'raw',
                    'fis': 'oper',
                    'fia': 'none',
                    'depth': 5, }

    def proc(self):
        return self.var


class ProcessURLTimeline(ProcessURL):

    def __init__(self, url):
        super(ProcessURLTimeline, self).__init__(url)

    def proc(self):
        url = self.url
        self.var['path'] = url[2:]
        return self.var


class ProcessURLV1(ProcessURL):

    def __init__(self, url):
        super(ProcessURLV1, self).__init__(url)

    def proc(self):
        url = self.url
        for num in xrange(len(url)):
            item = str(url[num])
            if num >= len(URL):
                continue
            self.var[URL[num]] = item
            if URL[num] == 'disp':
                self.var['path'] = '/' + item
                self.var['suite'] = item
        if self.var['kind'] != 'suite':
            if len(url) > 6:
                path = '/'.join(url[7:])
                if len(path) > 0:
                    if not path[0] in ('/', '_'):
                        path = '/' + path
                self.var['path'] = path
        return self.var

URL = ('http', 'api',  # 'version',
       'scheduler', 'host', 'port',
       'kind', 'disp', 'fis', 'fia', 'depth', 'suite', )


class ProcessURLV2(ProcessURL):

    def __init__(self, url):
        super(ProcessURLV2, self).__init__(url)

    def proc(self):
        url = self.url
        path = '/'
        self.var['suite'] = '_all_'
        for num, item in enumerate(url):
            if num >= len(URL):
                break
            self.var[URL[num]] = str(item)

        name = self.var['suite']
        if self.var['kind'] == 'suite':
            if len(url) > 11:
                xxx = str(url[10])
                if len(xxx) > 0:
                    self.var['suite'] = xxx
                    if xxx[0] in ('/', '_'):
                        path = xxx
                    else:
                        path = '/' + '/'.join(url[11:])
                else:
                    pass
            else:
                pass
            if name in ('_all_', ):
                path = '/' + name
            elif path == '/':
                path = '/' + name
            elif num >= len(URL):
                path = '/' + name + '/' + '/'.join(url[len(URL):])
            else:
                pass

        elif len(url) > 5:
            path = '/'.join(url[6:])
        else:
            pass

        self.var['path'] = path
        return self.var


class Command(object):

    def __init__(self, var=None):
        self.var = dict()
        self.var['host'] = 'localhost'
        self.var['port'] = 31415
        self.var['scheduler'] = 'ecflow'

        if type(var) is dict:
            for key in var.keys():
                self.var[key] = var[key]

    def server(self, sep=':'):  # aka contact
        return '%s%c%d' % (self.var['host'], sep, self.var['port'])


def red(text):
    return "<font color='red'>%s</font>" % text


class Servers(Command):

    def __init__(self, clt=None):
        super(Servers, self).__init__(clt)

    def response(self, request):
        sched.store(request)

        select_servers = "<select name='server'>\n"
        for nick in sched.servers().keys():
            item = sched.servers()[nick]
            select_servers += "<option value='%s:%s'" % (
                item['host'],
                item['port'])
            if nick == 'eode':
                select_servers += ' selected'
            select_servers += '>%s</option>\n' % nick
        select_servers += '</select>\n'
        select_actions = "<select name='action'>"
        for opt in SERVER_CMD:
            select_actions += "<option value='%s'>%s</option>\n" % (
                opt, opt.capitalize())
        select_actions += '</select>\n'

        form = dt.Template(form_template)
        br = '<br/>'
        body = '<h2>Another ecFlow Client</h2>'

        TOP = 'url syntax:' + br + 'http://%s/api/ecflow/' % HOST
        body += TOP + 'servers/treemap' + br

        TOP = 'http://%s/api/ecflow/' % HOST
        hosturl = TOP + '%s/%s' % (red(os.getenv('ECF_HOST', 'localhost')),
                                   red(os.getenv('ECF_PORT', str(ecf_port()))))
        hurl = TOP + client
        body += hosturl + '/<b>show</b>' + br
        body += hosturl + \
            '/suite/%s/<b>display/statuses/attributes/depth</b><br/>' % (
                red('test'))

        body += hosturl + \
            '/suite/%s/<b>display/statuses/attributes/depth</b><br/>' % (
                red('test'))

        body += br + '<b>show</b>:' + \
            ' '.join(SERVER_CMD) + ', suite name may be _all_'
        body += br + '<b>display</b>:' + ' '.join(sorted(DSUITE.keys()))
        body += br + '<b>statuses</b>: SARQCU all'
        body += br + '<b>attributes</b>: all none atTd<br/>'
        path = '/o/main/00/an/vardata'

        def pather(cmd):
            if cmd in ('script', 'man', 'job', 'output', 'timeline'):
                return cmd + path
            return cmd
        body += '''<div class='form-group'>
<label for='sel1'>%s/</label> ''' % hosturl + '''
<div class="form-group">
  <label for="comment">node:</label>
  <textarea class="form-control" rows="1" id="path">%s</textarea>
</div>
<select class='form-control' id='dyn'>
<option value='' selected>select</option>
    ''' % path + ' '.join([''' <option value='%s/%s'>%s</option>''' % (
            hurl,
            pather(cmd), cmd) for cmd in SERVER_CMD]) + '''
  </select>
<script>
    $(function(){
      $('#dyn').on('change', function () {
          // var url = $(this).val();
          var url = $(this).val().replace('%s', $('#path').val());
          if (url) { window.location = url;}
          return false;      });    });
</script>
<br>
<!--input class='SubmitButton' type='submit' name='SUBMIT' value='Submit'
    style='font-size:10px; '/-->
</div>'''
        man = 'http://%s/api/ecflow/%s/man' % (HOST, client) + path
        body += br + "man example: <a href='%s'>%s</a>" % (man, man)

        var = 'http://%s/api/ecflow/%s/suite/find/DATE/o' % (HOST, client)
        body += br + "find example: <a href='%s'>%s</a>" % (var, var)

        cont = {'title': 'Please, select a server',
                'body': body,
                'select_servers_list': select_servers,
                'select_actions_list': select_actions, }
        return HttpResponse(form.render(dt.Context(cont)))


@csrf_protect
def get_disp(request):
    for disp in sorted(DSUITE.keys()):
        if disp in request.path_info:
            break
        elif disp in parse_qs(request.META['QUERY_STRING']).keys():
            break
    return disp


class ServersT(Servers):

    def __init__(self, clt=None):
        super(ServersT, self).__init__(clt)

    def response(self, request):
        sched.store(request)
        context = {
            'title': 'Please, select a server',
            'jsondata': '"%s"' % sched.resturl("servers/json"),
            'json_url': sched.resturl("servers/json"),
            'items': [{"key": "raw",
                       "href": sched.resturl("servers/raw"), }, ],
            'json_times': "", }
        if 'suite' in self.var.keys():
            context['json_times'] = sched.resturl(
                contact(self.var, '/') + "/suite/times/%s" %
                self.var['suite'])

        try:
            template = dt.loader.get_template(
                'suite_%s.html' % get_disp(request))
        except:
            template = dt.loader.get_template('suite_treemap.html')
        return HttpResponse(template.render(context))


class ServersJson(Servers):

    def __init__(self, clt=None):
        super(ServersJson, self).__init__(clt)

    def response(self, request):
        sched.store(request)
        kids = []
        num = 0

        for key, item in sched.servers().items():
            num += 1
            kids.append({
                'name': key,
                'host': item['host'],
                'port': item['port'],
                'url': sched.resturl(contact(item, '/') + "/suites/treemap"),
                'meta': '',
                'size': num, })
        return HttpResponse(json.dumps({'name': '/', 'children': kids, }),
                            content_type="application/json")


class ServersJPing(Servers):

    def __init__(self, clt=None):
        super(ServersJPing, self).__init__(clt)

    def response(self, request):
        sched.store(request)
        kids = []
        num = 0

        for key, item in sched.servers().items():
            num += 1
            kids.append({
                'name': key,
                'host': item['host'],
                'port': item['port'],
                'url': sched.resturl(contact(item, '/') + "/suites/treemap"),
                'meta': sched.ping(contact(item)),
                'size': num, })
        return HttpResponse(json.dumps({'name': '/', 'children': kids, }),
                            content_type="application/json")


class ServersPing(Servers):

    def __init__(self, clt=None):
        super(ServersPing, self).__init__(clt)

    def response(self, request):
        sched.store(request)
        context = {
            'title': 'Please, select a server',
            'jsondata': '"%s"' % sched.resturl("servers/json"),
            'json_url': sched.resturl("servers/json"),
            'json_times': "", }
        try:
            template = dt.loader.get_template(
                'suite_%s.html' % get_disp(request))
        except:
            template = dt.loader.get_template('suite_treemap.html')
        return HttpResponse(template.render(context))


class Suites(Command):

    def __init__(self, clt=None):
        super(Suites, self).__init__(clt)

    def response(self, request):
        res = "suites"
        # select_actions_list = ""
        sal = "<select name='suite'><option value='jonas' selected>"
        sal += "</option></select>\n"
        try:
            sal = "<select name='suite'>\n"
            for item in sched.suites():
                sal += "<option value='%s'" % item
                if item == "jonas":
                    sal += " selected='selected' "
                sal += ">%s</option>" % item
            sal += "</select>\n"
        except BaseException, e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            res += "%s %s %s %s" % (e.args, exc_type, exc_tb.tb_lineno, fname)
        if res != "suites":
            return HttpResponse(res)

        form = dt.Template(form_template)
        cont = {
            'title': "Please, select a suite",
            'date_time': time.ctime(),
            'select_servers_list': "<select name='server'>" +
            "<option value='%s' selected>%s</option></select>" % (
                self.server(),
                self.server()),
            'select_actions_list': sal, }
        return HttpResponse(form.render(dt.Context(cont)))


class SuitesT(Suites):

    def response(self, request):
        sched.store(request)
        ext = contact(self.var, '/') + "/suites/json"
        context = {
            'time': time.ctime(),
            'title': 'Please, select a suite',
            'jsondata': '"%s"' % sched.resturl(ext),
            'json_url': sched.resturl(ext),
            'json_times': "", }
        template = dt.loader.get_template('suite_treemap.html')
        return HttpResponse(template.render(context))


class SuitesJ(Suites):

    def response(self, request):
        sched.store(request)
        kids = []
        suites = sched.suites(self.var)
        suites.append("_all_")  # 201509
        for num, key in enumerate(suites):
            # print(num, key)
            kids.append({
                'name': key,
                'size': num,  # FIXME +1???
                'meta': "",
                'url':  sched.resturl(
                    contact(self.var, '/') +
                    "/suite/treemap/adm/none/7/%s" % key)})
        return HttpResponse(json.dumps({'name': '/', 'children': kids, }),
                            content_type="application/json")


class Suite(Command):

    def __init__(self, var):
        """ simple command for: login-get-show """
        super(Suite, self).__init__(var)
        sched.store(self.var['scheduler'])  # FIXME
        self.json = sched.resturl(contact(self.var, '/') + "/suite/json")
        for key in ('fis', 'fia', 'depth',):
            self.json += '/' + '%s' % self.var[key]
        self.json += '%s' % self.var['path']

    def response(self, request):
        sched.store(request)
        name = self.var['suite']
        return HttpResponse(sched.login_get_show(contact(self.var), name))


class SuiteT(Suite):

    def __init__(self, clt):
        super(SuiteT, self).__init__(clt)

    def response(self, request):
        kind = get_disp(request)
        first = '/'.join(request.path_info.split('/')[:3])
        if '/api/api' in first:
            raise BaseException
        context = {
            'jsondata': '"%s"' % self.json,
            'json_url': self.json,
            'title': kind.capitalize(),
            'time': time.ctime(),
            'top': "http://%s" % HOST + first + "/servers/treemap",
            'json_times': sched.resturl(
                contact(self.var, '/') +
                "/suite/times/%s" % self.var['suite']),
            'items': [],
        }
        for key in sorted(DSUITES.keys()):
            context['items'].append({
                "key": key,
                "href": request.path.replace(kind, key)})
        fis_mode = None
        for key in FIS:
            if "/%s/" % key in request.path:
                fis_mode = key
        if fis_mode:
            for key in FIS:
                context['items'].append({
                    "key": key,
                    "href": request.path.replace(fis_mode, key)})

        try:
            template = dt.loader.get_template('suite_%s.html' % kind)
        except:
            template = dt.loader.get_template('suite_treemap.html')
        return HttpResponse(template.render(context))


class SuiteJ(Suite):

    def __init__(self, clt):
        super(SuiteJ, self).__init__(clt)

    def response(self, request):
        sched.store(request)
        return sched.json(self.var)


class SuiteV(Suite):

    def __init__(self, clt):
        super(SuiteV, self).__init__(clt)

    def response(self, request):
        return HttpResponse("<br/>".join(
            sched.find(contact(self.var), request.path_info)))


class SuiteTimes(Suite):

    def __init__(self, clt):
        super(SuiteTimes, self).__init__(clt)

    def response(self, request):
        return HttpResponse(
            json.dumps(sched.times(contact(self.var), request.path_info)))


FIS = ('oper', 'adm', 'SAR', 'SARQCUP', 'ops')

DSERVERS = {'treemap': ServersT,
            'sunburst': ServersT,
            'sunburs2': ServersT,
            'radial': ServersT,
            'json': ServersJson,
            'ping': ServersPing,
            'jp': ServersJPing,
            'raw': Servers, }  # ALWAYS

DSUITES = {'treemap': SuitesT,
           'sunburst': SuitesT,
           'sunburs2': SuitesT,
           'radial': SuitesT,
           'zoomt': SuitesT,
           'test': SuitesT,  # 'threem': SuitesT,
           'icicle': SuitesT,
           'json': SuitesJ,
           'raw': Suites, }

DSUITE = {'treemap': SuiteT,
          'sunburst': SuiteT,
          'sunburs2': SuiteT,
          'radial': SuiteT,
          # 'icicle': SuiteT,
          'dnttree': SuiteT,
          'zoomt': SuiteT,
          'test': SuiteT,  # 'threem': SuiteT,
          'icicle': SuiteT,
          'json': SuiteJ,
          'find': SuiteV,
          'times': SuiteTimes,
          'raw': Suite, }


def display_dispatch(request, dct=None, var=None):
    out = None
    for key in dct.keys():
        if key in request.path_info:
            if not dct[key]:
                continue
            out = dct[key](var).response(request)
            break
    if out:
        return out
    else:
        print('################', request.path_info, var)
    return dct['treemap'](var).response(request)


@csrf_protect
def process(request, *args, **kwargs):
    var = parse_qs(request.META['QUERY_STRING'])
    var.update(ProcessURLV1(request.path_info).proc())
    try:
        if var['depth'] not in xrange(10):
            var.update(ProcessURLV2(request.path_info).proc())
    except:
        var.update(ProcessURLV2(request.path_info).proc())
    # print(var)
    if var['kind'] == 'servers':
        return display_dispatch(request, DSERVERS)

    elif var['kind'] == 'suites':
        return display_dispatch(request, DSUITES, var)

    path = var['path']
    if path[0] != '/':
        path = '/' + path
    translate = {'stat': '--stats',
                 'log': '--group=log=get 100',
                 'history': '--group=log=get 100',
                 'ping':  '--ping',
                 'msg': '--msg %s' % path,
                 'script':  '--file %s script 10000' % path,
                 'job':  '--file %s job 10000' % path,
                 'output':  '--file %s jobout 10000' % path,
                 'man':  '--file %s manual 10000' % path,
                 # 'suites': 'suites',
                 'zomb': '--zombie_get',
                 'why': '--why %s' % path,
                 'mig': '--migrate %s' % path,
                 'var': '--migrate %s' % path,
                 'trig': '--migrate %s' % path,
                 'dep': '--migrate %s' % path,
                 'users': '--ch_suites', }

    for key in translate.keys():
        cmd = translate[key]
        if cmd is None:
            continue
        elif key in var['kind']:
            out = get_client_output(
                ec.Client(var['host'] + ':' + var['port']), str(cmd))
            return HttpResponse(out)

    return display_dispatch(request, DSUITE, var)


def escap(text):
    return mark_safe(force_unicode(text)
                     .replace('&', '&amp;')
                     .replace('<', '&lt;')
                     .replace('>', '&gt;')
                     .replace('"', '&quot;')
                     .replace("'", '&#39;'))


def get_client_output(clt, command='--ping'):
    res = '<h2>%s for server %s</h2>' % (command, contact(clt))
    if ';' in command:
        return 'injection ' + command
    cmd = [CLIENT, '--port', clt.get_port(), '--host', clt.get_host()]
    if 'why' in command:     # few shall NOT be split
        path = command.replace('--why ', '')
        cmd.append("--group='get 100; why /%s'" % path)
        out = os.popen(" ".join(cmd)).read()
        if isinstance(out, str):
            res += "<pre>" + escap(out) + "</pre>"
        return res
    else:
        cmd += command.split()
    cmd.append(";\n")
    res += "<p>" + " ".join(cmd) + "<br/>"
    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
    out, err = proc.communicate("")
    if isinstance(out, str):
        res += "<pre>" + escap(out) + "</pre>"
        res += "<pre>" + escap(err) + "</pre>"
    return res


DEF_SMS = False
DEF_NODE = "localhost@31415"
DEF_PATH = "/"
DEF_NODE = client.replace("/", "@")
DEF_PATH = "/e_xx"


def form(request, *args, **kwargs):
    template = dt.loader.get_template('form.html')
    context = {
        'path': "fsobs",
        'server': DEF_NODE + "@%d" % DEF_PORT,
        'status': "submitted-active-aborted", }
    for arg in args:
        context[arg] = arg
    for key in kwargs.keys():
        context[key] = kwargs[key]
    return HttpResponse(template.render(dt.Context(context)))

if DEF_SMS:
    def_place = "suite s; family f; task t; edit SMSCMD \"trimurti %SMSJOB% %SMSJOBOUT%\""
else:
    def_place = "suite s; family f; task t; edit ECF_JOB_CMD 'sub %ECF_JOB% %ECF_JOBOUT%'"


class SimpleFileForm(forms.Form):
    node = forms.CharField(initial=DEF_NODE, max_length=20)
    path = forms.CharField(initial=DEF_PATH, max_length=20)
    sms = forms.BooleanField(required=False, initial=DEF_SMS)
    raw = forms.BooleanField(required=False, initial=DEF_SMS)
    save = forms.BooleanField(required=False, initial=False)
    filn = forms.FileField(label="Select an expended.def file", required=False)
    expended = forms.CharField(
        widget=forms.Textarea({
            'rows': 3, "text": " suite s",
            'placeholder': def_place, }),
        required=False,)


def handle_file(f):
    out = ""
    for chunk in f.chunks():
        # print(chunk)
        out += chunk
    return out


def tostr(source):
    import unicodedata
    return unicodedata.normalize('NFKD', source).encode('ascii', 'ignore')


def sms2ecf(text, is_sms=False):
    if not is_sms:
        return text
    return text.replace("edit SMS", "edit ECF_").replace(
        " %SMS", " %ECF_").replace(
        "ECF_CMD", "ECF_JOB_CMD").replace(
            "ECF_KILL", "ECF_KILL_CMD").replace(
            "ECF_STATUSCMD", "ECF_STATUS_CMD").replace(
            "smshostfile", "ecf_hostfile").replace(
            "sms_hosts", "ecf_hosts").replace(
                "set SMS_PROG ", "# set SMS_PROG").replace(
                "Welcome to", "# Welcome to").replace("Goodbye ", "# Goodbye ").replace(
                "ECF_URLCMD", "ECF_URL_CMD")


# def handle_uploaded_file(content):
#     with tempfile.NamedTemporaryFile(mode="w+t",
#                                      dir= "/tmp/%s" % get_username()) as temp:
#         for chunk in content.chunks():
#             temp.write()
#         out = def2def.process(fname= temp.name)
#     return out


# @csrf_protect
# def def2def(request): # , *args, **kwargs):
#     icon = dt.RequestContext(request)
#     template = dt.loader.get_template('def2def.html')
#     if 0: context = dt.RequestContext({
#         'path':   "emc_xx",
#         'server': client.replace("/", "@"),
#         'status': "unknown", })
#     template = dt.loader.get_template("def2def.html")
#     saving = ""
#     file = ""
#     output = ""
#     content = ""
#     if request.POST:
#         import def2def
#         form = SimpleFileForm(request.POST, request.FILES)
#         if form.is_valid():
#             is_sms = form.cleaned_data['sms']
#             is_raw = form.cleaned_data['raw']
#             is_save = form.cleaned_data['save']
#             if is_save:
#                 content = "Content-Disposition: attachment; filename='fname"
#                 if is_raw:
#                     content += ".exp'"
#                 else: content += ".py'"

#             node, port = tostr(form.cleaned_data['node']).split("@")
#             path = form.cleaned_data['path']
#             exp = form.cleaned_data['expended']
#             print(request.POST.keys())
#             if exp != "":
#                 xxx = sms2ecf(tostr(exp), is_sms)
#                 if not is_raw: output = def2def.process(desc= xxx.splitlines())
#                 else: output = xxx
#                 saving = "# from string description"
#             elif request.POST['filn']:
#                 print(request.POST.keys(), request.FILES.keys())
#                 saving = "# from file %s" % request.POST['filn']
#                 ndoc = None
#                 ndoc = models.FDoc(dfile= request.POST['filn']); ndoc.save()
#                 if 0:
#                     output += def2def.process(fname= tostr(ndoc.filename()))
#                 elif 0: # if len(request.FILES.keys()) > 0:
#                   lines = ""
#                   for key, fil in request.FILES.items():
#                       file = form.cleaned_data['port'],
#                       saving += "# " + fil.name
#                       if fil.multiple_chunks:
#                         for c in fil.chunks():
#                             lines += tostr(c)
#                         else: lines += tostr(fil.read())
#                       if is_sms: lines = sms2ecf(lines, 1)
#                       if is_raw:
#                           output += lines.rstrip()
#                       else: output += def2def.process(string= lines.rstrip())
#                       output += "# lines"
#             elif is_sms:
#                 cdp = "/usr/local/apps/sms/bin/cdp"
#                 proc = Popen(
#                     [cdp, "-q", "-c",
#                      "set SMS_PROG %s; login %s %s 1; get %s; show %s;" % (
#                          port, node, get_username(), path, path) ],
#                     stdout= PIPE, stderr= PIPE)
#                 cmd = "set SMS_PROG %s; login %s %s 1; get %s; show %s;" % (
#                     port, node, get_username(), path, path)
#                 out, err = proc.communicate("")
#                 if is_raw:
#                     output = "%s\n%s" % (sms2ecf(out, 1), err ) + "\n# " + cmd
#                 else: output = def2def.process(desc= sms2ecf(out, 1))

#                 saving = "#from sms: %s@%s" % (node, port) + path
#             else:
#                 # import unicodedata
#                 saving = "# from server:" + path
#                 # unicodedata.normalize('NFKD', path).encode('ascii', 'ignore')
#                 try:
#                     if is_raw:
#                         cmd = [CLIENT, "--port", port, "--host", node,
#                                "--group",
#                                "get %s; show" % path ]
#                         proc = Popen(cmd, stdout= PIPE, stderr= PIPE)
#                         out, err = proc.communicate("")
#                         output += "%s\n#err:" % out + err
#                     else:
#                         path= tostr(node + "@%s%s" % (port, path))
#                         output += def2def.process(path= path)
#                 except RuntimeError as e:
#                     out = "#WAR: failed: " + str(e)
#                     output += "# server is not responding" \
#                              + " or path not found " + path
#                     print(out, output)
#                 output += "# server"
#         else: pass
#     else: form = SimpleFileForm()
#     icon.update({'form':   form,
#                  'status': "",
#                  'output': output,
#                  'file': file,
#                  'saving': saving })
#     icon.update(csrf(request))
#     return HttpResponse(template.render(icon),
#                         content_type= content) # "application/json")


def compare(request, *args, **kwargs):
    template = dt.loader.get_template("." + request.path_info)

    context = {
        'path': "fsobs",
        'server': "localhost@31415",
        'status': "submitted-active-aborted", }
    for arg in args:
        context[arg] = arg
    for key in kwargs.keys():
        context[key] = kwargs[key]
    return HttpResponse(template.render(context))


def def_doc(request, *args, **kwargs):
    doc_template = """<!DOCTYPE html>
<html><head>
<meta http-equiv=”refresh” content=”60" >
<meta charset="utf-8">
        <!-- jQuery should be at least version 1.7 -->
        <script src="http://code.jquery.com/jquery-1.11.0.min.js"></script>
        <script src="/static/assets/contextmenu.js"></script> 
        <link rel="stylesheet" type="text/css" href="/static/assets/contextmenu.css" />

<link rel="stylesheet" type="text/css" href="/static/assets/sms.css" />
<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.11.2/jquery.min.js"></script>
<script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.2/js/bootstrap.min.js"></script>
<script type="text/javascript" src="/static/assets/sms.js"></script>
<link rel="stylesheet" href="http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css">
<script src="http://code.jquery.com/jquery-1.11.3.min.js"></script>
<script src="http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js"></script>
<style>
img {
    max-width: 100%;
    max-height: 100%
}
</style>
</head>
<body>

<div data-role="page" id="pageone">
  <div data-role="header">
    <h1>Definition files Py-Doc</h1>
  </div>

  <div data-role="main" class="ui-content">
    <ul data-role="listview">
      <li><a href="static/ecf.html" data-ajax="false">ecf.py</a>
<li><a 
    href="https://software.ecmwf.int/wiki/display/ECFLOW/ecFlow+Python+Api"
    data-ajax="false"> native ecFlow API</a></li>
<li><a href="static/o.html" data-ajax="false">o</a>
<li><a href="static/mc.html" data-ajax="false">mc</a>
<li><a href="static/law.html" data-ajax="false">law</a>
<li><a href="static/tc3.html" data-ajax="false">tc3</a>
<li></li>
<li><a href="static/inc_prod.html" data-ajax="false">inc_prod</a>
<li><a href="static/inc_pop.html" data-ajax="false">inc_pop</a>
<li><a href="static/inc_reserve.html" data-ajax="false">inc_reserve</a>
<li><a href="static/inc_legc.html" data-ajax="false">inc_legc</a>
<li><a href="static/s2s.html" data-ajax="false">s2s</a>
<li></li>
<li><a href="static/inc_an.html" data-ajax="false">inc_an</a>
<li><a href="static/inc_obs.html" data-ajax="false">inc_obs</a>
<li><a href="static/inc_stream.html" data-ajax="false">inc_stream</a>
<li><a href="static/inc_fam.html" data-ajax="false">inc_fam</a>
<li></li>
<li><a href="static/doxy/html/index.html" data-ajax="false">doxygen</a></li>
<li><a href="static/htmlcov/index.html" data-ajax="false">coverage</a></li>
<li><a href="static/clones.html" data-ajax="false">clones</a></li>
<li><a href="static/parameters.html" data-ajax="false">parameters</a>
</ul>

<p>main modules relations
  <a href="static/out.dot.pdf" data-ajax="false">
  <img src="static/out.dot.png" alt="click me" /></a>

<p><table><tr>
<td> <a href="static/o.pdf" data-ajax="false">
    <figure>
     <img src="static/o.dot.png" alt="o" width="150px" />
    <figcaption>o.def</figcaption>    </figure>
</a></td>
<td> <a href="static/mc.pdf" data-ajax="false">
    <figure>
     <img src="static/mc.dot.png" alt="mc"  width="150px" />
    <figcaption>mc</figcaption>    </figure>
</a></td>
<td> <a href="static/law.pdf" data-ajax="false">
    <figure>
     <img src="static/law.dot.png" alt="law"  width="150px" />
    <figcaption>law</figcaption>    </figure>
</a></td>
<td> <a href="static/tc3.pdf" data-ajax="false">
    <figure>
     <img src="static/tc3.dot.png" alt="tc3"  width="150px" />
    <figcaption>tc3</figcaption>    </figure>
</a></td>
<td> <a href="static/longrange.pdf" data-ajax="false">
    <figure>
     <img src="static/longrange.dot.png" alt="seas4"  width="150px" />
    <figcaption>seas4</figcaption>    </figure>
</a></td>
</tr></table>

<p>ecf.py classes hierarchy
  <img src="static/classes_ecf.png" alt="ecf" />
<p>inc_an (functional) classes hierarchy
  <img src="static/classes_inc_an.png" alt="inc_an" />
<p>inc_stream classes hierarchy
  <img src="static/classes_inc_stream.png" alt="inx_stream" />
<p>inc_obs classes hierarchy
  <img src="static/classes_inc_obs.png" alt="inc_obs" />
<p>inc_fam classes hierarchy
  <img src="static/classes_inc_fam.png" alt="inc_fam" />
<p>inc_libs classes hierarchy
  <img src="static/classes_inc_libs.png" alt="inc_libs" />
<p>inc_prod classes hierarchy
  <img src="static/classes_inc_prod.png" alt="inc_prod" />

<p> example from mc.def: suites defined from "system class hisrarchy"
  <img src="static/classes_mc.png" alt="mc" />
<p>example from o.def: suites defined from "system class hisrarchy"
  <img src="static/classes_o.png" alt="o" />
</div>
</body>
</html>
"""

    template = dt.Template(doc_template)
    context = {
        'path': "fsobs",
        'server': "localhost@31415",
        'status': "submitted-active-aborted", }
    for arg in args:
        context[arg] = arg
    for key in kwargs.keys():
        context[key] = kwargs[key]
    return HttpResponse(template.render(context))


DEF_NODE = 'localhost'
DEF_PORT = 31415
DEF_PORT = ecf_port()
DEF_PATH = '/'
URLS = ['admin', 'include', 'timeline', 'triggers', 'vars',
        'tasks', 'diff',
        ]


class SimpleForm(forms.Form):
    node = forms.CharField(initial=DEF_NODE, max_length=20)
    port = forms.IntegerField(initial=DEF_PORT)
    path = forms.CharField(initial=DEF_PATH, max_length=200)
    task = forms.CharField(initial=DEF_PATH, max_length=200, required=0)
    kind = forms.CharField(initial='script', max_length=20,  required=0)


has_relation = dict()

forget = ('complete',
          'queued',
          'aborted',
          '!=',           # '==',
          'ne', 'eq', 'gt', 'ge', 'lt', 'le',
          'and', 'not', '(', ')', 'AND', 'OR', 'EQ', 'NE',
          'or',)


def add_to_relation(path):
    global has_relation
    has_relation[path] = ()


def trigger2nodes(node, top=''):
    trg = node.get_trigger()
    if not trg:
        return []
    dad = node.get_parent().get_abs_node_path()
    trigger = '%s' % trg
    out = []
    for elt in trigger.replace('==', ' ').split():
        if elt in forget:
            continue
        try:
            a = int(elt)
            continue  # elt is integer
        except:
            pass
        elt = elt.replace('==complete', '').replace(
            '(', '').replace(')', '')
        if ':' in elt:
            elt, event = elt.split(':')
        while elt[0:3] == '../':
            dad = '/'.join(dad.split('/')[:-1])
            elt = elt[3:]
        while elt[0:2] == './':
            elt = elt[2:]

        if len(elt) > 0:
            if elt[0] == '/':
                path = elt
            else:
                path = dad + '/' + elt

            out.append(path.replace(top, ''))
            add_to_relation(path)
    add_to_relation(dad)
    return out


def node2relation(defs, node, top=''):
    out = []
    if not node:
        print("#WAR: node is None")
        return out
    if top[-1] != '/':
        top += '/'
    path = node.get_abs_node_path()
    for item in defs.get_all_nodes():
        if path not in item.get_abs_node_path():
            continue
        short = item.get_abs_node_path().replace(top, "")
        imports = trigger2nodes(item, top)
        if short not in has_relation.keys() and len(imports) == 0:
            continue
        out.append({"name": short,
                    "size": 1,
                    "imports": imports})
    return out


def tojson(request):
    kind = request.path_info.split('/')[2]
    node, port = request.path_info.split('/')[3].split('@')
    path = request.path_info.replace("/json/%s/" % kind, "")
    defs, item = clt.todef(node, port, path)
    load = {}
    if kind == "include":
        load = clt.get_node_include(defs, item)
    elif kind in ("triggers", "timeline"):
        load = node2relation(defs, item, item.get_abs_node_path())
    return HttpResponse(json.dumps(load),
                        content_type="application/json")


def edge(request):
    template = dt.loader.get_template('edge.html')
    icon = dt.RequestContext(request)
    return HttpResponse(template.render(icon))


def d3d(request):
    template = dt.loader.get_template('d3d.html')
    icon = dt.RequestContext(request)
    return HttpResponse(template.render(icon))


login_template = """
{% load staticfiles %}
{% load bootstrap3 %}
{% include "header.html" %}
<!--DOCTYPE html><html><head></head><body-->

  <noscript>
  <h3>JavaScript is required for this web site</h3>
  <span>Please enable JavaScript in your browser before continuing</span>
  </noscript>
  <!--div id="heading">
    <div class="align_left fbold">Sign in</div>
    <div class="float_right colorgrey30">ECFLOW</div>
  </div>
  <form name="login_form" method="post" action="/login">
 {% csrf_token %} 
    <noscript>
      <input type="hidden" name="noscript" value="1" />
    </noscript>

    <label for='login_username'>
      Username<br />
      <input style="width: 100%" size="25" id="login_username" name="login_username" value='""" + get_username() + """' />
    </label>
    <label for='login_password'>
      Password<br />
      <input style="width: 100%" size="25" id="login_password" type="password" name="login_password"
      placeholder="Leave blank to use your certificate."/>
    </label>
    <br />
  <input type="hidden" name="back" value="" /> 
  <input type="hidden" name="app" value="ECFLOW" />
  <div class="formButtons">
  <input name='login' id='login' type="submit" value="Sign in" />
  </div>
  </form-->

<form action="/login" method="POST">
 {% csrf_token %} 
<table border="0" width="100%"  height="600px">
<tr height="550px">
<td align="center">
<!--table style="background-color:#fff3b5; border: 1px solid #cecf9c; padding: 8px; align:center "-->
<table style="border: 1px solid #cecf9c; padding: 8px; align:center ">
<tr><td class="form_title">User</td>
<td><input name="username" class="identifier"/></td></tr>

<tr><td class="form_title">Pass</td>
<td><input type="password" name="password" class="identifier"/></td></tr>
<tr><td></td>
<td><input type="submit" value="Go!"/></td></tr>
</table>
</td>
</tr>
<tr height="50px">
<td valign="top" align="center">&nbsp;</td>
</tr>
</table>
</form>

{{ body }}
</body>
</html>
"""

cmd = """
dest=/tmp/$USER/mofc/verify; 
from=/gpfs/lxop/emos_data/mofc/0001/plots;
rm -rf $dest/lx*; mkdir -p $dest/lxab $dest/lxop $dest/lxc;
scp -r emos@lxop:$from/mon/verify $dest/lxop/mon;
scp -r emos@lxop:$from/thu/verify $dest/lxop;
scp -r emos@lxab:/gpfs/lxab/emos_data/mofc/0001/plots/verify $dest/lxab;

from=/ws/scratch/ma/emos/lxc/emos_data/mofc/0001/plots/
scp -r emos@lxc:$from/verify $dest/lxc
scp -r emos@lxc:$from/mon/verify $dest/lxc
scp -r emos@lxc:$from/thu/verify $dest/lxc

module load gnuparallel
cat > $HOME/topng.sh<<@@
#!/bin/bash
PS2PNG=/home/ma/map/bin/ps2png
file=\$1
echo $file ${file%.ps}.png
$PS2PNG $file ${file%.ps}.png
@@
chmod 755 $HOME/topng.sh

PS2PNG=/home/ma/map/bin/ps2png
for file in $(find $dest -type f -name "*.ps" -print); do
  [[ ! -s $file ]] && continue; echo $file
  [[ -s ${file%.ps}.png ]] && continue
#  $PS2PNG   $file ${file%.ps}.png &
echo $file
  # sleep 1
done > l.tmp
# cat l.tmp | parallel $HOME/topng.sh
cat l.tmp | parallel --jobs 4 $HOME/topng.sh ::: 
cat l.tmp | parallel --jobs 10 $PS2PNG {} {}.png ::: 
cat l.tmp | parallel --jobs 10 echo $PS2PNG {} {}.png ::: 

wait 
"""


def process_form(form):
    try:
        task = form.cleaned_data['task']
    except:
        task = form.cleaned_data['path']
    try:
        kind = form.cleaned_data['kind']
    except:
        kind = "script"
    return (form.cleaned_data['host'],
            form.cleaned_data['port'],
            form.cleaned_data['path'],
            task, kind, )


@csrf_protect
def refresh(request):
    proc = Popen(cmd.split())
    out, err = proc.communicate("")
    return cmd


@csrf_protect
def login(request):
    form = dt.Template(login_template)
    action = "Login"
    if request.POST:
        username = request.POST['username']
        password = request.POST['password']
        user = authenticate(username=username, password=password)
        if user is not None:
            if user.is_active:
                if 1:
                    return verify(request)  # Redirect to a success page.
            else:
                output = 'disabled account'
        else:
            action = "invalid login"
    icon = {'title': "Login, please",
            'body': "", 'action': action, }
    icon.update(csrf(request))
    return HttpResponse(form.render(icon))


@csrf_protect
def triggers(request):
    context = {}
    template = dt.loader.get_template('generic.html')
    edge = dt.loader.get_template('edge.html')
    output = ""
    full = ''
    action = "triggers"
    erpng = False
    defs = None
    item = None
    if request.POST:
        form = SimpleForm(request.POST, request.FILES)
        if form.is_valid():
            node, port, path, task, kind = process_form(form)
            if '/' in task:
                full = "%s@%s%s" % (node, port, path)
                defs, item = clt.todef(node, port, path)
            if item:
                output = "trigger= %s" % item.get_trigger()
            if item:
                rels = node2relation(defs, item, item.get_abs_node_path()),
                from graphviz import Digraph
                styles = {'graph': {'label': path,
                                    'fontsize': "9",
                                    'fontcolor': "black",
                                    'rankdir': "LR", }
                          }
                e = Digraph("frel", filename="frel.gv", engine='neato')
                e.graph_attr.update(
                    ('graph' in styles and styles['graph']) or {})
                e.attr("node", shape="box")
                e.attr("node", shape="box")
                with open("flow/template/frel.html", "w") as fout:
                    for xx in rels:
                        for item in xx:
                            name = item['name']
                            for trg in item['imports']:
                                print("'%s' > '%s'" % (trg, name), file=fout)
                                e.edge(trg, name)
                e.save("er.dot")
                os.system("dot -y -Tpng er.dot > flow/static/er.png")
                erpng = True
                output += "\nload= " + pprint.pformat(rels, indent=2)
            else:
                output += "# node not found"
        else:
            pass
    else:
        form = SimpleForm()
    icon = dict()  # dt.RequestContext(request)
    icon.update(csrf(request))
    icon.update({'form': form,
                 'urls': URLS,
                 'edge': "%s" % edge.render(icon),
                 'path': full,  # "%s@%s%s" % (node, port, path),
                 'erpng': erpng,
                 'action': action,
                 'textarea': output  # + "%s" % dico,
                 })
    return HttpResponse(template.render(icon))


@csrf_protect
def include(request):
    context = {}
    template = dt.loader.get_template('generic.html')
    output = ""
    full = ''
    action = "include"
    display = ""
    defs = None
    item = None
    items = None
    if request.POST:
        form = SimpleForm(request.POST, request.FILES)
        if form.is_valid():
            node, port, path, task, kind = process_form(form)
            if '/' in task:
                full = "%s@%s%s" % (node, port, path)
                defs, item = clt.todef(node, port, task)
                items = clt.get_tasks(defs, item)
                items.append(path)

            clt.listfiles.clear()
            output = pprint.pformat(clt.get_node_include(defs, item),
                                    indent=2)
            display = pprint.pformat(clt.listfiles.report(), indent=2)
    else:
        form = SimpleForm()
    icon = dt.RequestContext(request)
    icon.update({'form':   form,
                 "urls": URLS,
                 # 'edge': "",
                 'path': full, 'items': items,
                 'action': action,
                 'textarea': "%s" % output,
                 'display': display,
                 })
    icon.update(csrf(request))
    return HttpResponse(template.render(icon))


@csrf_protect
def tasks(request):
    context = {}
    template = dt.loader.get_template('generic.html')
    output = ""
    full = ''
    action = "tasks"
    display = ""
    items = [DEF_PATH, ]
    kind = ['manual', 'edit', 'preprocess',
            'script', 'job', 'output',
            'vars', 'include', ]
    choice = 'script'

    if request.POST:
        form = SimpleForm(request.POST, request.FILES)
        if form.is_valid():
            node, port, path, task, choice = process_form(form)
            if '/' in task:
                full = '%s@%s%s' % (node, port, task)
                defs, item = clt.todef(node, port, task)
                items = clt.get_tasks(defs, item)
                items.append(path)
            if choice == 'edit':
                output, err = clt.edit(node, port, task, 0)
            elif choice == 'preprocess':
                output, err = clt.edit(node, port, task, 1)
            elif choice in ('script', 'output', 'job', 'manual'):
                output, err = clt.file(node, port, task, choice)
            elif choice == 'vars':
                clt.listfiles.clear()
                output = pprint.pformat(
                    clt.get_node_vars(defs, item, node, port),
                    indent=2)
                display = clt.listfiles.report()
            elif choice == 'include':
                clt.listfiles.clear()
                output = pprint.pformat(clt.get_node_include(defs, item),
                                        indent=2)
                display = pprint.pformat(clt.listfiles.report(), indent=2)

            items.append(path)
    else:
        form = SimpleForm()
    icon = dt.RequestContext(request)
    icon.update({'form': form,
                 'urls': URLS,
                 'path': full, 'items': items, 'rows': 100,
                 'action': action, 'kind': kind, 'choice': choice,
                 'textarea': '%s' % output,
                 'display': display, })
    icon.update(csrf(request))
    return HttpResponse(template.render(icon))


@csrf_protect
def vars(request):
    template = dt.loader.get_template('generic.html')
    output = ''
    full = '/'
    action = 'vars'
    if request.POST:
        form = SimpleForm(request.POST, request.FILES)
        if form.is_valid():
            node, port, path, task, choice = process_form(form)
            full = '%s@%s%s' % (node, port, path)
            defs, item = clt.todef(node, port, path)
            clt.listfiles.clear()
            output = pprint.pformat(
                clt.get_node_vars(defs, item, node, port),
                indent=2)
    else:
        form = SimpleForm()
    icon = dt.RequestContext(request)
    icon.update({'form': form,
                 'urls': URLS,
                 'edge': '',
                 'path': full,
                 'action': action,
                 'textarea': '%s' % output, })
    icon.update(csrf(request))
    return HttpResponse(template.render(icon))


sched = Scheduler()

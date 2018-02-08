#!/usr/bin/env python
# coding: utf-8
""" demo for alternative display for ecFlow suites
  started with django framework                201502
  shifted to Flask, if it can make it lighter  201504
"""
from __future__ import print_function    # (at top of module)
from gevent import monkey
monkey.patch_all()

import time
import datetime
import os
import re
from subprocess import Popen, PIPE
from threading import Thread
from flask import Flask, render_template, session, request
from flask.ext.socketio import SocketIO, emit, join_room, leave_room, \
    close_room, disconnect
from flask.ext.cache import Cache

from subprocess import Popen, PIPE
from threading import Thread
from flask import Flask, render_template, session, request, Markup, flash
from flask.ext.socketio import SocketIO, emit
from flask.ext import restful
from flask_cors import CORS
from cgi import parse_qs
import jinja2 as dt
import json
import sys
ecf_py = '/usr/local/apps/ecflow/current/lib/python2.7/site-packages/'
# ecf_py)
sys.path.append('/usr/local/apps/ecflow/current/lib/python2.7/site-packages/')
try:
    import ecflow as ec
except ImportError:
    sys.path.append('/usr/local/lib/python2.7/site-packages')
    import ecflow as ec

SLEEP_INTERVAL = 120
DEBUG = 0

app = Flask(__name__)
app.debug = True
app.config['SECRET_KEY'] = 'secret!'
ns = '/ecflow'

socketio = SocketIO(app)
thread = None
thread2 = None
NOW = time.gmtime()
cache = Cache(app, config={'CACHE_TYPE': 'simple', })
# cache.init_app(app)

CLIENTS = dict()
# cache.set('mdef', None)


def process(item, depth=0, count=0):
    defs = None
    # defs = cache.get('mdef')
    if type(item) == str:
        defs = ec.Defs(item)

    elif isinstance(item, ec.Client):
        NOW = int(time.strftime('%H%M%S', time.gmtime()))
        ident = "%s:%s" % (item.get_host(), item.get_port())
        if ident not in CLIENTS.keys():
            CLIENTS[ident] = NOW
            item.sync_local()
            # cache.set('mdef', item.get_defs())
        elif NOW - CLIENTS[ident] > 180 or NOW - CLIENTS[ident] < -1000:  # midnight
            item.sync_local()
            # cache.set('mdef', item.get_defs())
        # else:            print(CLIENTS[ident], NOW, ident)
        defs = item.get_defs()
        # defs= cache.get('mdef')
    elif isinstance(item, ec.Defs):
        # defs= cache.get('mdef')
        defs = item

    if defs:
        count = 0
        for node in defs.suites:
            status = "%s" % node.get_state()
            if node.is_suspended():
                status = "suspended"
                continue
            if status == "unknown":
                continue
            socketio.emit('suites list',
                          {'node': {'_id': count,
                                    'name': node.name(),
                                    'alert': 0,
                                    'status': status}},
                          # {'data': msg, 'count': count},
                          namespace=ns)
            for item in node.nodes:
                count = process(item, depth + 1, count)
    elif isinstance(item, ec.Family):
        for node in item.nodes:
            count = process(node, depth + 1, count)
    elif isinstance(item, ec.Task):
        node = item
        status = "%s" % node.get_state()
        if node.is_suspended():
            status = "suspended"
        if status not in ("aborted", "submitted", "active"):
            return count
        evt = ""
        meter = ""
        stamp = ""
        try:
            stamp = "%s" % node.get_state_change_time().replace(
                "2016-", "").replace(
                "2017-", "").replace(
                "2018-", "").replace("T", "").replace(":", "").replace(
                "01-", "").replace(
                "02-", "").replace(
                "03-", "").replace(
                "04-", "").replace(
                "05-", "").replace(
                "06-", "").replace(
                "07-", "").replace(
                "08-", "").replace(
                "09-", "").replace(
                "10-", "").replace(
                "11-", "").replace(
                "12-", "")

            for att in node.events:
                if att.value():
                    evt = att.name()
            for att in node.meters:
                meter = " %s:%s" % (att.name(), att.value())
                meter = " %d" % att.value()
        except Exception as excpt:
            print("#! problem with line:", excpt)
        name = "%s %-20s %-5s %-5s" % (stamp, node.get_abs_node_path(),
                                       evt, meter)

        # gvar = ec.VariableList()
        # node.get_generated_variables(gvar)
        # for var in gvar:
        # print name, status, count
        NOW = time.gmtime()
        now = time.strftime('%d%H%M', NOW)
        if status == "submitted":
            alert = int(now) - int(stamp) > 12
        elif status == "aborted":
            alert = int(now) - int(stamp) > 500
        elif status == "active":
            alert = int(now) - int(stamp) > 200
        # if status == "active" and not alert: pass
        run = AddLog(item, 1)
        if status == "active":
            run.add_job_eta(name)
        if node.get_abs_node_path() != "":
            # if status == "active" and alert: alert = False
            # self.run()
            socketio.emit('tasks list',
                          {'node': {'_id': count,
                                    'name': name + run.log,
                                    'alert': alert,
                                    'status': status}},
                          namespace=ns)
            count += 1
            AddLog(item, count)

    elif isinstance(item, ec.Alias):
        pass

    elif isinstance(item, tuple):
        for pawn in item:
            process(pawn, depth, count)

    else:
        print(type(item))
    return count

ECF = "/usr/local/apps/ecflow/current"

servers = {"ecflow": [ECF + "/share/ecflow/servers",
                      ECF + "/bin/ecflow_client --host %s --ping", ],
           "sms": ["/usr/local/apps/sms/current/lib/servers",
                   "/usr/local/apps/sms/bin/smsping %s", ],
           }

rpat = "^(?P<nick>[a-zA-Z0-9\-_@:\.]+)[ \t]+(?P<host>[a-zA-Z0-9\-_]+)[ \t]+(?P<port>[0-9]+)$"


def init_servers():
    recp = re.compile(rpat)
    for key in servers.keys():
        try:
            with open(servers[key][0], 'r') as fid:
                try:
                    for line in fid:
                        match = recp.match(line.rstrip())
                        if line == "":
                            continue
                        elif match is None:
                            continue
                        if match:
                            servers[key].append(match.groups())
                except Exception as excpt:
                    print("#! problem with line:", line, excpt)
        except:
            pass


def background_thread():
    """Example of how to send server generated events to clients."""
    global NOW
    count = 0
    eode = ec.Client("%s:%s" % (os.getenv("ECF_HOST", "localhost"),
                                os.getenv("ECF_PORT", "2500")))
    while True:
        count += 1
        NOW = time.gmtime()
        if 1:
            socketio.emit('suites list',
                          {'node': {'_id': count,
                                    'name': "%s current time" %
                                    time.strftime('%d%H%M', NOW),
                                    'alert': 0,
                                    'status': "submitted"}},
                          namespace=ns)
        process(eod2)
        count = process(eod1)
        count = process(eod3)
        count = process(eode)
        if 0:
            socketio.emit(
                'my response',
                {'data': 'Server generated event', 'count': count},
                namespace='/test')
        time.sleep(SLEEP_INTERVAL)
        socketio.emit('clear', {}, namespace=ns)


def updater(tree, iid, name, value):
    pass


@app.route('/')
@cache.cached(timeout=60)
def index():
    global thread
    if thread is None:
        thread = Thread(target=background_thread)
        thread.start()
    return render_template('index.html')


@socketio.on('my event', namespace='/test')
def test_message(message):
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response',
         {'data': message['data'], 'count': session['receive_count']})


@socketio.on('my broadcast event', namespace='/test')
def test_broadcast_message(message):
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response',
         {'data': message['data'], 'count': session['receive_count']},
         broadcast=True)


@socketio.on('join', namespace='/test')
def join(message):
    join_room(message['room'])
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response',
         {'data': 'In rooms: ' + ', '.join(request.namespace.rooms),
          'count': session['receive_count']})


@socketio.on('leave', namespace='/test')
def leave(message):
    leave_room(message['room'])
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response',
         {'data': 'In rooms: ' + ', '.join(request.namespace.rooms),
          'count': session['receive_count']})


@socketio.on('close room', namespace='/test')
def close(message):
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response', {'data': 'Room ' + message['room'] + ' is closing.',
                         'count': session['receive_count']},
         room=message['room'])
    close_room(message['room'])


@socketio.on('my room event', namespace='/test')
def send_room_message(message):
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response',
         {'data': message['data'], 'count': session['receive_count']},
         room=message['room'])


@socketio.on('disconnect request', namespace='/test')
def disconnect_request():
    session['receive_count'] = session.get('receive_count', 0) + 1
    emit('my response',
         {'data': 'Disconnected!', 'count': session['receive_count']})
    disconnect()


@socketio.on('connect', namespace='/test')
def test_connect():
    emit('my response', {'data': 'Connected', 'count': 0})


@socketio.on('connect', namespace=ns)
def test_ecflow_connect():
    emit('my response', {'data': 'Connexion', 'count': 0})


@socketio.on('disconnect', namespace='/test')
def test_disconnect():
    print('Client disconnected')

# Thanks!
# https://requests-oauthlib.readthedocs.org/en/latest/
# from requests_oauthlib import OAuth2Session
from flask import Flask, request, redirect, session, url_for
from flask.json import jsonify


@app.route("/login")
def login():
    github = OAuth2Session(client_id)
    authorization_url, state = github.authorization_url(authorization_base_url)
    # State is used to prevent CSRF, keep this for later.
    session['oauth_state'] = state
    return redirect(authorization_url)


@app.route("/callback")
def callback():
    github = OAuth2Session(client_id, state=session['oauth_state'])
    token = github.fetch_token(token_url, client_secret=client_secret,
                               authorization_response=request.url)
    return jsonify(github.get('https://api.github.com/user').json())


CLIENT = "ecflow_client"
CLIENT = "/usr/local/apps/ecflow/current/bin/ecflow_client"

DJANGO = 0
app = Flask(__name__)
app.debug = True
app.config['SECRET_KEY'] = 'secret!'
app.config['CORS_HEADERS'] = 'Content-Type'
cors = CORS(app, resource={r'/api/ecflow/': {"origins": "*"}})
api = restful.Api(app)
ns = '/ecflow'

socketio = SocketIO(app)
thread = None

import platform
PORT = 5003
HOST = platform.node() + ".ecmwf.int:%d" % PORT
APPE = "http://%s/api/ecflow/" % HOST
APPS = APPE.replace("ecflow", "sms")

FIS = ("oper", "adm", "SAR", "SARQCUP")

SERVER_CMD = ("suites", "stats", "ping", "history", "zombie",
              "script", "man", "job", "output", "timeline",
              # "info", "msg", "var", "why",
              )
COMPRESS = 0
# http://flask.pocoo.org/snippets/56/
# from datetime import timedelta
# from flask import make_response, request, current_app
# from functools import update_wrapper


# def crossdomain(origin=None, methods=None, headers=None,
#                 max_age=21600, attach_to_all=True,
#                 automatic_options=True):
#     if methods is not None:
#         methods = ', '.join(sorted(x.upper() for x in methods))
#     if headers is not None and not isinstance(headers, basestring):
#         headers = ', '.join(x.upper() for x in headers)
#     if not isinstance(origin, basestring):
#         origin = ', '.join(origin)
#     if isinstance(max_age, timedelta):
#         max_age = max_age.total_seconds()

#     def get_methods():
#         if methods is not None:
#             return methods

#         options_resp = current_app.make_default_options_response()
#         return options_resp.headers['allow']

#     def decorator(f):
#         def wrapped_function(*args, **kwargs):
#             if automatic_options and request.method == 'OPTIONS':
#                 resp = current_app.make_default_options_response()
#             else:
#                 resp = make_response(f(*args, **kwargs))
#             if not attach_to_all and request.method != 'OPTIONS':
#                 return resp

#             h = resp.headers

#             h['Access-Control-Allow-Origin'] = origin
#             h['Access-Control-Allow-Methods'] = get_methods()
#             h['Access-Control-Max-Age'] = str(max_age)
#             if headers is not None:
#                 h['Access-Control-Allow-Headers'] = headers
#             return resp

#         f.provide_automatic_options = False
#         return update_wrapper(wrapped_function, f)
#     return decorator
# /

def display_dispatch(request, dct, var=None):
    # print(request.path, dct.keys(), var)
    for key, item in dct.items():
        if DJANGO:
            if "path_info" not in request.__dict__.keys():
                continue
            elif key in request.path_info:
                if not item:
                    continue
                return item(var).response(request)
        elif key in request.path and item:
            return item(var).response(request)
    return dct["treemap"](var).response(request)


def contact(client, sep=':'):
    res = "localhost%c31415" % sep
    if isinstance(client, dict):
        if "localhost" == str(client['host']):
            client['host'] = "localhost"
        res = "%s%c%s" % (client['host'], sep, client['port'])
    elif isinstance(client, ec.Client):
        res = "%s%c%s" % (client.get_host(), sep, client.get_port())
    return res


class Command(object):

    def __init__(self, var=None):
        self.var = dict()
        self.var["host"] = "localhost"
        self.var["port"] = "31415"

        if type(var) is dict:
            for key in var.keys():
                self.var[key] = var[key]

    def server(self, sep=':'):  # aka contact
        return "%s%c%s" % (self.var["host"], sep, self.var["port"])

    def response(self, request):
        sched.store(request.path, self.var)
        try:
            self.context = RequestContext(request, None)
        except:
            self.context = dict()

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


class Servers(Command):

    def __init__(self, clt=None):
        super(Servers, self).__init__(clt)

    def response(self, request):
        super(Servers, self).response(request)

        select_servers = "<select name='server'>\n"
        for nick in sched.servers().keys():
            item = sched.servers()[nick]
            select_servers += "<option value='%s:%s'" % (item['host'],
                                                         item['port'])
            if nick == "eode":
                select_servers += " selected"
            select_servers += ">%s</option>\n" % nick
        select_servers += "</select>\n"
        select_actions = "<select name='action'>"
        for opt in SERVER_CMD:
            select_actions += "<option value='%s'>%s</option>\n" % (
                opt, opt.capitalize())
        select_actions += "</select>\n"

        if "/sms" in request.path:
            APPH = APPS
        else:
            APPH = APPE

        form = dt.Template(form_template(request).decode('utf-8'))
        br = "<br/>"
        body = "<h2>Another ecFlow Client</h2>"
        TOP = "url syntax: " + APPH
        body += TOP + "servers/treemap" + br

        TOP = "url syntax: " + APPH
        body += TOP + "%s/%s/<b>show</b>" % (red("vsms2"), red("43333")) + br

        body += TOP + "%s/%s/suite/%s/<b>display/statuses/attributes/depth</b><br/>" % (
            red("vsms2"), red("43333"), red("emc_41r1"))
        body += br + "<b>show</b>:" + \
            " ".join(SERVER_CMD) + ", suite name may be _all_"
        body += br + "<b>display</b>:" + " ".join(DISPLAYS)
        body += br + "<b>statuses</b>: SARQCU all"
        body += br + "<b>attributes</b>: all none atTd<br/>"

        man = APPH + "vsms2/43333/man/emc_41r1/main/12/legA/fc/cf/modeleps_nemo"
        body += br + "man example: <a href='%s'>%s</a>" % (man, man)

        var = APPH + "vsms2/43333/suite/find/SCHOST/emc_41r1"
        body += br + "find example: <a href='%s'>%s</a>" % (var, var)

        var = APPS + "ablamor/314159/timeline"
        body += br + "timeline: <a href='%s'>%s</a>" % (var, var)

        cont = {'title': "Please, select a server",
                'body': body,
                'select_servers_list': select_servers,
                'select_actions_list': select_actions, }
        if DJANGO:
            return HttpResponse(form.render(cont))
        else:
            return form.render(cont)


def get_template(name):
    try:
        out = loader.get_template(name)
    except:
        out = app.jinja_env.get_template(name)
    return out


class ServersT(Servers):

    def __init__(self, clt=None):
        super(ServersT, self).__init__(clt)

    def response(self, request):
        super(Servers, self).response(request)
        if DJANGO:
            context = RequestContext(request, None)
        else:
            context = self.context

        url = sched.resturl("servers/json")
        if "/sms" in request.path:
            url = url.replace("/ecflow", "/sms")

        context['title'] = "Please, select a server"
        context['jsondata'] = "\"" + url + "\""
        context['json_url'] = url

        if "suite" not in self.var.keys():
            context['json_times'] = sched.resturl(
                contact(self.var, '/') + "/suite/times/%s" % self.var['suite'])
        else:
            context['json_times'] = ""

        try:
            template = get_template('suite_%s.html' % get_disp(request))
        except:
            template = get_template('suite_treemap.html')
        if DJANGO:
            return HttpResponse(template.render(context))
        else:
            return template.render(context)


class ServersJson(Servers):

    def __init__(self, clt=None):

    def response(self, request):
        super(ServersJson, self).response(request)
        kids = []
        num = 0
        for key, item in sched.servers().items():
            num += 1
            kids.append({
                'name': key,
                'host': item['host'],
                'port': item['port'],
                'url': sched.resturl(contact(item, '/') + "/suites/treemap"),
                'meta': "",
                'size': num, })
        if DJANGO:
            return HttpResponse(
                json.dumps({'name': '/',  'children': kids, }),
                content_type="application/json")
        else:
            return json.dumps({'name': '/',  'children': kids, })


class ServersJPing(Servers):

    def __init__(self, clt=None):
        super(ServersJPing, self).__init__(clt)

    def response(self, request):
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
        if DJANGO:
            return HttpResponse(
                json.dumps({'name': '/',  'children': kids, }), content_type="application/json")
        else:
            return json.dumps({'name': '/',  'children': kids, })


class ServersPing(Servers):

    def __init__(self, clt=None):
        super(ServersPing, self).__init__(clt)

    def response(self, request):
        super(ServersPing, self).response(request)
        if DJANGO:
            context = self.context
        else:
            context = self.context
        context['title'] = "Please, select a server"

        url = sched.resturl("servers/jp")
        if "/sms" in request.path:
            url = url.replace("/ecflow", "/sms")

        context['jsondata'] = "\"" + url + "\""
        context['json_url'] = url
        context['json_times'] = ""
        try:
            template = get_template('suite_%s.html' % get_disp(request))
        except:
            template = get_template('suite_treemap.html')
        if DJANGO:
            return HttpResponse(template.render(context))
        else:
            return template.render(context)


class Suites(Command):

    def __init__(self, clt=None):
        uper(Suites, self).__init__(clt)

    def response(self, request):
        super(Suites, self).response(request)
        res = "suites"
        select_actions_list = ""
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
            return res  # HttpResponse(res)

        form = dt.Template(form_template(request).decode('utf-8'))
        # cont = dt.Context({
        context = {
            'title': "Please select a suite",
            'date_time': time.ctime(),
            'select_servers_list': "<select name='server'>" +
            "<option value='%s' selected>%s</option></select>" % (
                self.server(),
                self.server()),
            'select_actions_list': sal, }
        # return HttpResponse(form.render(context))
        return form.render(context)


class SuitesT(Suites):

    def response(self, request):
        super(SuitesT, self).response(request)
        ext = contact(self.var, '/') + "/suites/json"
        if DJANGO:
            context = RequestContext(request, None)
        else:
            context = self.context
        context['title'] = "Please, select a suite"
        context['time'] = time.ctime()

        url = sched.resturl(ext)
        if "/sms" in request.path:
            url = url.replace("/ecflow", "/sms")

        context['jsondata'] = "\"" + url + "\""
        context['json_url'] = url
        context['json_times'] = ""
        template = get_template('suite_treemap.html')
        if DJANGO:
            return HttpResponse(template.render(context))
        else:
            return template.render(context)


class SuitesJ(Suites):

    def response(self, request):
        super(SuitesJ, self).response(request)
        kids = []
        for num, key in enumerate(sched.suites(self.var)):
            kids.append({
                'name': key,
                'size': num,
                'meta': "",
                'url':  sched.resturl(
                    contact(self.var, '/') + "/suite/treemap/adm/none/7/%s" % key)
            })

        load = {'name': '/',  'children': kids, }
        if DJANGO:
            return HttpResponse(json.dumps(load),
                                content_type="application/json")
        else:
            return json.dumps(load)


class Suite(Command):

    def __init__(self, var):
        """ simple command for: login-get-show """
        super(Suite, self).__init__(var)
        sched.store("ecflow")  # FIXME

        self.json = sched.resturl(contact(self.var, '/') + "/suite/json")
        for key in ("fis", "fia", "depth",):
            self.json += '/' + "%s" % self.var[key]
        self.json += "%s" % self.var['path']

    def response(self, request):
        super(Suite, self).response(request)
        name = self.var["suite"]
        if DJANGO:
            return HttpResponse(sched.login_get_show(contact(self.var), name))
        else:
            return sched.login_get_show(contact(self.var), name)


class SuiteT(Suite):

    def __init__(self, clt):
        super(SuiteT, self).__init__(clt)

    def response(self, request):
        super(SuiteT, self).response(request)
        kind = get_disp(request)
        if DJANGO:
            context = RequestContext(request, None)
        else:
            context = self.context  # RequestContext(request, None)
        if "/sms" in request.path:
            APPH = APPS
        else:
            APPH = APPE

        context['jsondata'] = "\"" + self.json + "\""
        context['json_url'] = self.json
        # context['title'] = " ".join(DSUITES.keys()) + ", " + " ".join(FIS)
        context['title'] = kind.capitalize()
        context['time'] = time.ctime()
        context['top'] = APPH + "servers"
        context['json_times'] = sched.resturl(
            contact(self.var, '/') + "/suite/times/%s" % self.var['suite'])
        context['items'] = []
        for key in DSUITES.keys():
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
            template = get_template('suite_%s.html' % kind)
        except:
            template = get_template('suite_treemap.html')
        if DJANGO:
            return HttpResponse(template.render(context))
        else:
            return template.render(context)


class SuiteJ(Suite):

    def __init__(self, clt):
        super(SuiteJ, self).__init__(clt)

    def response(self, request):
        super(SuiteJ, self).response(request)
        if DJANGO:
            RequestContext(request, None)
        else:
            context = self.context
        context['jsondata'] = "\"" + self.json + "\""
        context['json_url'] = self.json
        sched.store(request)
        return sched.json(self.var)


class SuiteV(Suite):

    def __init__(self, clt):
        super(SuiteV, self).__init__(clt)

    def response(self, request):
        if DJANGO:
            return "<br/>".join(sched.find(contact(self.var),
                                           request.path_info))
        else:
            return "<br/>".join(sched.find(contact(self.var),
                                           request.path))


class SuiteTimes(Suite):

    def __init__(self, clt):
        super(SuiteTimes, self).__init__(clt)

    def response(self, request):
        if DJANGO:
            return json.dumps(sched.times(contact(self.var),
                                          request.path_info))
        else:
            return json.dumps(sched.times(contact(self.var),
                                          request.path))


DSERVERS = {"treemap": ServersT,
            "sunburst": ServersT,
            "sunburs2": ServersT,
            "radial": ServersT,
            "json": ServersJson,
            "ping": ServersPing,
            "jp": ServersJPing,
            "raw": Servers,  # ALWAYS
            }

DSUITES = {"treemap": SuitesT,
           "sunburst": SuitesT,
           "sunburs2": SuitesT,
           "radial": SuitesT,
           "json": SuitesJ,
           "raw": Suites,

           "icicle": SuiteT,
           "dnttree": SuiteT,

           }

DSUITE = {"treemap": SuiteT,
          "sunburst": SuiteT,
          "sunburs2": SuiteT,
          "radial": SuiteT,
          "icicle": SuiteT,
          "dnttree": SuiteT,
          "json": SuiteJ,
          "find": SuiteV,
          "times": SuiteTimes,
          "raw": Suite,
          }


URL = ('http', 'api',  # 'version',
       'scheduler', 'host', 'port',
       'kind', 'disp', 'fis', 'fia', 'depth', 'suite', )


def process(request, *args, **kwargs):
    if DJANGO:
        var = parse_qs(request.META['QUERY_STRING'])
        path_info = request.path_info
        var.update(ProcessURLV1(path_info).proc(args))
        try:
            num = int(var['depth'])
        except:
            var.update(ProcessURLV2(path_info).proc(args))

    else:
        var = dict()
        path_info = request.path
        var.update(ProcessURLV2(path_info).proc(args))

    if var["kind"] == "servers":
        return display_dispatch(request, DSERVERS)

    elif var["kind"] == "suites":
        return display_dispatch(request, DSUITES, var)

    elif var["kind"] == "timeline":
        return timeline(request, var)

    path = var["path"]
    translate = {
        "stat": "--stats",
        "log": "--log=get",
        "history": "--log=get",
        "ping":  "--ping",
        "msg": "--msg %s" % path,

        "script":  "--file %s script 10000" % path,
        "job":  "--file %s job 10000" % path,
        "output":  "--file %s jobout 10000" % path,
        "man":  "--file %s manual 10000" % path,

        # "suites": "suites",
        "zomb": "--zombie_get",
        "why": "--why %s" % path,
        "mig": "--migrate %s" % path,

        "var": "--migrate %s" % path,
        "trig": "--migrate %s" % path,
        "dep": "--migrate %s" % path,

        "users": "--ch_suites",
    }

    for key, cmd in translate.items():
        if key in var["kind"]:
            if cmd is None:
                continue
            client = ec.Client(var["host"] + ":" + var["port"])
            out = get_client_output(client, str(cmd))
            if DJANGO:
                return HttpResponse(out)
            else:
                out = Markup(out)
                flash(out)
                return render_template("output.html")

    return display_dispatch(request, DSUITE, var)


form_templateX = """<!DOCTYPE html>
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

if 0:
    form_template += """<form 
  name="ecflow.form" 
  action='action='%s'
  onsubmit='return gotoPage();'
  method='get'>
<b>{{ title }}</b>
{{ select_servers_list | safe}}
{{ select_actions_list | safe }}
<!--input type="submit" value="Submit" onclick="form_feed();"-->
<input type="submit" value="Submit" onclick="gotoPage();">
<button onclick="gotoPage();">Send</button>
<br />
<script type="text/javascript">
createBoxStatuses(1);
// document.write('<br />');
createBoxAttributes(1);
// document.write('<br />');
createBoxDisplays(1, checked='{{ display | safe }}');
</script>
</form>"""  # % APPH

form_templateY = """</div>
<p>{{ body | safe}}
<div id="test1">
<a href="%sservers/treemap" 
   class="test">ecFlow servers</a>
<a href=%sservers/treemap" 
   class="test">SMS servers</a><br/>

<a href="%svsms2/31415/suite/sunburst/all/none/5/einterim"
   class="test">sunburst</a>
<a href="%svsms2/31415/suite/radial/all/none/5/einterim"
   class="test">radial</a><br/>
<a href="%svsms2/43333/suite/sunburs2/SAR/none/5/e_41r1"
   class="test">e-suite</a>
<a href="%svsms2/43333/suite/radial/SAR/none/5/eeda_41r1"
   class="test">eeda</a>
<a href="%svsms2/43333/suite/sunburst/SAR/none/5/elaw_41r1"
   class="test">elaw</a>
<a href="%svsms2/43333/suite/sunburst/SAR/none/5/emc_41r1"
   class="test">emc</a>
<a href="%svsms2/43333/suite/treemap/SAR/none/5/eeda_41r1"
   class="test">eeda-t</a><br/>
<a href="%svsms2/43333/suite/treemap/SARQCU/non/5/macc"
   class="test">macc</a>
<a href="%svsms2/43333/suite/sunburst/SARQCU/none/5/macc"
   class="test">macc-s</a>
<a href="%svsms2/32222/suite/sunburst/adm/none/7/fsobs" 
   class="test">fsobs</a>
<!--a href="%svsms1/433334/suite/raw/SAR/none/7/emc_crayr" class="test">emc_crayr</a-->
        </div>
<!-- initially hidden right-click menu -->
<!--div class="hide" id="rmenu">
<ul><li> <a href="http://www.google.com">Google</a>    
</li><li> <a href="http://%s/login">Localhost</a>
</li><li> <a href="C:\">C</a></li></ul> </div-->
</body>
</html>"""  # % (APPH, APPH, APPH, APPH, APPH, APPH, APPH, APPH, APPH,
#              APPH, APPH, APPH, APPH, APPS, )


def form_template(request):
    if "/sms" in request.path:
        APPH = APPS
    else:
        APPH = APPE

    return form_templateX + form_templateY % (
        APPH, APPH, APPH, APPH, APPH, APPH, APPH, APPH, APPH,
        APPH, APPH, APPH, APPH, APPS, )


class GenericScheduler(object):

    def __init__(self):
        self.servers_list = None
        self.serv = dict()
        self.name = "generic"
        self.req = "/"
        self.rest = APPE
        self.clients = dict()

    def store(self, req, dct=dict()):
        try:
            if isinstance(req, django.core.handlers.wsgi.WSGIRequest):
                if DJANGO:
                    req = req.path_info
                else:
                    req = req.path

        except:
            dct.keys()
        self.req = req

    def load_list(self):
        if not self.servers_list:
            raise
        with open(self.servers_list, 'r') as servers_file:
            for line in servers_file:
                nick, host, port = line.split()
                self.serv[nick] = {'host': host,
                                   'port': port, }

    def resturl(self, add=""):
        return self.rest + add

    def servers(self):
        if len(self.serv) == 0:
            self.load_list()
        return self.serv

    def client(self, key):
        return self.clients[key]

    def suites(self, key):
        return "cray"

    def ping(self, key):
        return "ping_unk1"


homes = ("/tmp",
         )


class JobTimes(object):

    def __init__(self, path):
        self.path = str(path.rstrip('/'))
        self.memo = dict()
        self._find_file(self.path)
        self._find_file(self.path + ".old")

    def _find_file(self, path="./", ext="job1"):
        memo = self.memo
        for root, dirs, files in os.walk(str(path)):
            if 0:
                print(root, "consumes",)
                print(sum([getsize(join(root, name)) for name in files]))
            for file in files:
                ffp = join(root, file)
                for home in homes:
                    path = ffp.replace(home, "")
                path = path.replace(".old", "")
                if not os.path.isfile(ffp):
                    continue
                (mode, ino, dev, nlink, uid, gid, size,
                 atime, mtime, ctime) = os.stat(ffp)
                try:
                    path, ext = path.split(".", 1)
                except:
                    continue
                if "." in path:
                    raise

                if path not in memo.keys():
                    memo[path] = dict()

                kind = "curr"
                if ".old" in ffp:
                    kind = "prev"
                if kind not in memo[path].keys():
                    memo[path][kind] = dict()

                if file.endswith(".orig"):
                    memo[path][kind]["created"] = time.ctime(mtime)
                    memo[path][kind]["size_job_ecf"] = getsize(join(ffp))
                elif file.endswith("job1.sub"):
                    memo[path][kind]["rid"] = time.ctime(mtime)
                elif file.endswith(".job1"):
                    memo[path][kind]["start"] = time.ctime(mtime)
                    memo[path][kind]["size_job_sub"] = getsize(join(ffp))
                elif file.endswith(".1"):
                    memo[path][kind]["stop"] = time.ctime(mtime)
                    memo[path][kind]["size_job_out"] = getsize(join(ffp))
                else:
                    pass
            else:
                pass
        else:
            pass
        self.memo = memo


class EcflowScheduler(GenericScheduler):

    def __init__(self):
        super(EcflowScheduler, self).__init__()
        self.name = "ecflow"
        self.servers_list = "/usr/local/apps/ecflow/current/share/ecflow/servers"
        self.rest = APPE
        # self.rest += self.name

    def client(self, key):
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
        if "Failed" in err:
            return "ping_nok " + key
        elif " succeeded" in out:
            return "ping_ok"
        return "ping_unk2 - %s - %s" % (out, err)

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
        content = "%s" % defs
        name += "<br/>" + content.replace("\n", "<br />")
        clt.ch_drop()
        return name

    def times(self, key, url):
        import def2json
        import pickle
        output = open("pik.out", "wb")

        if injection(url):
            return "nop"
        name = "ECF_HOME"
        suite = '/'
        path = '/'
        try:
            surl = url.split('/')
            name = "ECF_HOME"
            suite = str(surl[7])
            path = "/%s" % suite
            if len(url) > 8:
                path += '/' + '/'.join(surl[8:])
        except:
            pass

        clt = self.client(key)
        if not name in('/', "_all_", "", None):
            clt.ch_register(False, [suite, ])

        clt.sync_local()
        defs = clt.get_defs()
        clt.ch_drop()

        res = def2json.ListVarUse(defs, name).process(path)
        paths = dict()
        out = []
        for item in res:
            name, val = item.split(" ")
            paths[val] = ""
        for val in paths.keys():
            out.append(JobTimes(val + path).memo)

        pickle.dump(val + path, output)
        pickle.dump(out, output)
        output.close()
        return out[0]

    def find(self, key, url):
        import def2json
        if injection(url):
            return "nop"
        name = "ECF_HOME"
        suite = '/'
        path = '/'
        try:
            surl = url.split('/')
            name = str(surl[7])
            suite = str(surl[8])
            path = str("/%s/" % suite + '/'.join(surl[9:]))
        except:
            pass

        clt = self.client(key)
        if not name in('/', "_all_", "", None):
            clt.ch_register(False, [suite, ])

        clt.sync_local()
        defs = clt.get_defs()
        clt.ch_drop()

        proc = def2json.ListVarUse(defs, name)
        # return HttpResponse("<br/>".join(proc.process(path)))
        return proc.process(path)

    def json(self, var):
        import def2json
        import gzip
        import cStringIO
        clt = self.client(contact(var))

        items = (("suite", "jonas"),
                 ("fis", "oper"),
                 ("fia", "all"),
                 ("depth", "5"))
        for key, val in items:
            if key not in var.keys():
                var[key] = val

        res = "json"
        tree = None
        try:
            name = "_all_"
            if 'suite' in var.keys():
                name = var["suite"]
            if name != "_all_":
                clt.ch_register(False, [name, ])
            clt.sync_local()
            defs = clt.get_defs()
            if defs is None:
                res = "empty server %s" % contact(clt)
            if name != "_all_":
                clt.ch_drop()

            tree = def2json.TreeD3JS(defs,
                                     fis=var["fis"],
                                     fia=var["fia"],
                                     depth=var["depth"])
            jsontree = tree.to_json()
            if jsontree is None:
                jsontree = "none"
            out = "%s" % jsontree

            if COMPRESS:
                zbuf = cStringIO.StringIO()
                zfile = gzip.GzipFile(mode='wb', compresslevel=0, fileobj=zbuf)
                zfile.write(out.encode('utf-8'))
                zfile.close()
                content = zbuf.getvalue()
                res = HttpResponse(content)
                res['Content-Encoding'] = 'gzip'
                res['Content-Length'] = str(len(content))
                return res
            if DJANGO:
                return HttpResponse(out, content_type="application/json")
            else:
                return out

        except BaseException, e:
            # thanks
            # http://stackoverflow.com/questions/1278705/python-when-i-catch-an-exception-how-do-i-get-the-type-file-and-line-number
            exc_type, exc_obj, exc_tb = sys.exc_info()
            fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
            res = "%s %s %s %s" % (e.args, exc_type, exc_tb.tb_lineno, fname)
        if DJANGO:
            return HttpResponse(res)
        else:
            return res


NOCDP = 0
source = "/usr/local/apps/sms/lib/"
try:
    sys.path.append(source)
    import cdp
except ImportError:
    NOCDP = 1


class SMSClient(object):

    def __init__(self, key):
        self.host, port = key.split(":")
        self.port = int(port)
        node = cdp.sms_node()
        tree = cdp.sms_info_pointed(node)
        cdp.sms_numbers(self.port, 0)
        timeout = 60
        passwd = "1"
        user = get_username()
        self.handle, rc = cdp.sms_client_login(self.host, user, passwd,
                                               timeout, self.port, tree)
        self.num = -1
        self.top = None
        self.state = None


def get_username():
    import pwd
    import os
    return pwd.getpwuid(os.getuid())[0]


def injection(strs):
    for item in strs:
        if '$' in item or ';' in item:
            return True
    return False


class SMSScheduler(GenericScheduler):
    init = 0

    def __init__(self):
        super(SMSScheduler, self).__init__()
        self.name = "sms"
        self.servers_list = "/usr/local/apps/sms/lib/servers"
        # self.rest += self.name
        self.rest = APPS

    def client(self, key):
        if NOCDP:
            return None
        if not SMSScheduler.init:
            cdp.cdp_init([""], 0)
            SMSScheduler.init = 1
        else:
            pass
        if key not in self.clients.keys():
            self.clients[key] = SMSClient(key)
        return self.clients[key]

    def ping(self, key):
        host, prog = key.split(":")
        os.putenv("SMS_PROG", prog)
        proc = Popen(["smsping", host], stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate("")
        if "errorcode" in out:
            return "ping_nok " + key
        elif " OK!" in out:
            return "ping_ok"
        return "ping_unk2 - %s - %s" % (out, err)

    def suites(self, key):
        host, port = contact(key).split(":")
        user = "map"
        cdp = "/usr/local/apps/sms/bin/cdp"
        if injection((port, host, user)):
            return "dont do that"
        arg = "set SMS_PROG %s; login %s %s 1; suites" % (
            port, host, user)
        proc = Popen([cdp, "-q", "-c", arg, ";\n"], stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate("")
        res = ""
        ignore = True
        for line in out.split("\n"):
            if "Suites you" in line:
                ignore = True
                break
            elif not ignore:
                res += line
            elif "Suites defined" in line:
                ignore = False
        return [var for var in res.split(" ") if var]

    def login_get_show(self, key, name):
        host, port = key.split(":")
        user = "map"
        cdp = "/usr/local/apps/sms/bin/cdp"
        if injection((port, host, user, name)):
            return "dont do that"

        arg = "set SMS_PROG %s; login %s %s 1; get /%s; show /%s" % (
            port, host, user, name, name)
        proc = Popen([cdp, "-q", "-c", arg, ";\n"], stdout=PIPE, stderr=PIPE)
        out, err = proc.communicate("")
        return out.replace("\n", "<br />")

    def find(self, key, url):
        return "Not yet!"

    def times(self, key, url):
        return "Not yet!"

    def json(self, var):
        return "not yet!"


class Scheduler(GenericScheduler):
    """ proxy pattern, fronting for an implementation """

    def __init__(self):
        self.schedulers = (EcflowScheduler(), SMSScheduler())
        self.__impl = None
        self.__ecf = EcflowScheduler()
        self.__sms = SMSScheduler()
        super(Scheduler, self).__init__()
        self.name = "generic"

    def __getattr__(self, name):
        path = "%s" % self.req
        if self.__impl:
            return getattr(self.__impl, name)
        elif "/sms/" in path:
            return getattr(self.__sms, name)
        else:
            return getattr(self.__ecf, name)

    def load_list(self):
        self.__ecf.load_list()
        self.__sms.load_list()

    def servers(self):
        return self.__ecf.servers()

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
        return "ping_unk3"

    def login_get_show(self, key, name):
        for item in self.schedulers:
            if item.name in self.req:
                return item.login_get_show(key, name)
        return None

    def find(self, key, name):
        for item in self.schedulers:
            if item.name in self.req:
                return item.find(key, name)
        return "none"

    def times(self, key, name):
        for item in self.schedulers:
            if item.name in self.req:
                return item.times(key, name)
        return dict()

sched = Scheduler()


class ProcessURL(object):

    def __init__(self, url="/"):
        self.url = url.split('/')
        self.var = {"host": "localhost",
                    "port": "31415",
                    "kind": "servers",
                    "suite": "_all_",
                    "path": "/",
                    "display": "raw",
                    "fis": "oper",
                    "fia": "none",
                    "depth": 7,
                    }

    def proc(self, *args, **kwargs):
        if len(args) > 2:
            self.var.update(args)
        if len(kwargs) > 2:
            self.var.update(kwargs)

        return self.var


class ProcessURLTimeline(ProcessURL):

    def __init__(self, url):
        super(ProcessURLTimeline, self).__init__(url)

    def proc(self, *args, **kwargs):
        super(ProcessURLV1, self).proc(args, kwargs)
        url = self.url
        self.var["path"] = url[2:]
        return self.var


class ProcessURLV1(ProcessURL):

    def __init__(self, url):
        super(ProcessURLV1, self).__init__(url)

    def proc(self, *args, **kwargs):
        super(ProcessURLV1, self).proc(args, kwargs)
        url = self.url
        for num in xrange(len(URL)):
            item = str(url[num])
            if num >= len(url):
                break
            self.var[URL[num]] = item
            if URL[num] == 'disp':
                self.var["path"] = '/' + item
                self.var["suite"] = item
        if self.var["kind"] != "suite":
            if len(url) > 5:
                path = '/'.join(url[6:])
                if len(path) > 0:
                    if not path[0] in ('/', '_'):
                        path = '/' + path
                self.var["path"] = path

        return self.var

# URL = ("http", "api", # "version",
#       "scheduler", "host", "port",
#       "kind", "disp", "fis", "fia", "depth", "suite", )


class ProcessURLV2(ProcessURL):

    def __init__(self, url):
        super(ProcessURLV2, self).__init__(url)

    def proc(self, *args, **kwargs):
        super(ProcessURLV2, self).proc(args, kwargs)
        url = self.url
        path = '/'
        self.var['suite'] = '_all_'
        for num, item in enumerate(url):
            if num >= len(URL):
                break
            self.var[URL[num]] = str(item)

        name = self.var["suite"]
        if self.var["kind"] == "suite":
            if len(url) > 9:
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
            if name in ("_all_", ):
                path = name
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

        self.var["path"] = path
        return self.var


def red(text):
    return "<font color='red'>%s</font>" % text


def get_disp(request):
    for disp in DISPLAYS:
        if DJANGO:
            if disp in request.path_info:
                break
            elif disp in parse_qs(request.META['QUERY_STRING']).keys():
                break
        elif disp in request.path:
            break  # FLASK
    return disp


def escap(text):
    from jinja2.filters import do_mark_safe
    out = (text  # force_unicode(text)
           .replace('&', '&amp;')
           .replace('<', '&lt;')
           .replace('>', '&gt;')
           .replace('"', '&quot;')
           .replace("'", '&#39;'))
    if DJANGO:
        return do_mark_safe(force_unicode(text))
    else:
        return out


def get_client_output(clt, command="--ping"):
    res = "<h3>%s for server %s</h2>" % (command, contact(clt))
    cmd = [CLIENT, "--port", clt.get_port(), "--host", clt.get_host()]
    if "why" in command:     # few shall NOT be split
        path = command.replace("--why ", "")  # 'get; why ") + "'"
        cmd.append("--group='get ; why /%s'" % path)
        out = os.popen(" ".join(cmd)).read()
        if isinstance(out, str):
            res += "<pre>" + escap(out) + "</pre>"
        return res
    else:
        cmd += command.split()
    res += "<p>" + " ".join(cmd) + "<br/><p>"
    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
    out, err = proc.communicate("")
    if isinstance(out, str):
        res += "<pre>" + escap(out) + "</pre>"
        res += "<pre>" + escap(err) + "</pre>"
    return res


DISPLAYS = sorted(DSUITE.keys())
SLEEP_INTERVAL = 15


@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def index_node_port(ecflow="ecflow",
                    host="localhost", port="31415", path='/'):
    return process(request, {"kind": ecflow, "host": host, "port": port, })


@app.route('/api/ecflow/servers')
def index_servers(ecflow="ecflow", host="localhost", port="31415"):
    return process(request, {"kind": ecflow, "host": host, "port": port, })


@app.route('/api/ecflow/servers/json')
def index_servers_json(ecflow="ecflow", host="localhost", port="31415"):
    return ServersJson().response(request)


@app.route('/api/sms/servers')
def index_sms_servers(ecflow="sms", host="localhost", port="31415"):
    return process(request, {"kind": ecflow, "host": host, "port": port, })


@app.route('/api/sms/servers/json')
def index_sms_servers_json(ecflow="sms", host="localhost", port="31415"):
    return ServersJson().response(request)


if __name__ == '__main__':
    socketio.run(app, host="0.0.0.0", port=PORT)

"""
"""

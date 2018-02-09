# -*- coding: utf-8 -*-
import os
from django.db import models

STATUS_TABLE = [('unk', 'unknown'),
                ('sub', 'submitted'),
                ('act', 'active',),
                ('com', 'complete'),
                ('abt', 'aborted'),
                ]

"""
python manage.py shell
"""
class User(models.Model):
    name = models.CharField(max_length=50)

class Author(models.Model):
    name       = models.CharField(max_length=50)
    email       = models.CharField(max_length=50)
    lname       = models.CharField(max_length=50)
    createdBy   = models.ForeignKey(User)
    createdAt   = models.DateTimeField()

class Property(models.Model):
    name  = models.CharField(max_length=50)
    value = models.CharField(max_length=50)

class Comment(models.Model):
    created_on = models.DateTimeField(auto_now_add=True)
    author = models.CharField(max_length=255)
    author_email = models.EmailField()
    text = models.TextField()

def process_nc( nc, parent=None, server=""):
    for node in nc.nodes:     
        xxx = process_node(node, parent, server)
        if not isinstance(node, ecflow.Task):
            process_nc(node, xxx, server)

def add(path, kind, label=""):    
    if label is None: return

def process_node(node, parent=None, server=""):
        path = node.get_abs_node_path()
        name = node.name()
        if isinstance(node, ecflow.Family): kind= "family"
        elif isinstance(node, ecflow.Suite): kind= "suite"
        elif isinstance(node, ecflow.Task): kind= "task"
        else: kind= "unk"
        status = str(node.get_state())
        path = node.get_abs_node_path()

        print path, status
        from fdi.models import Node
        if 1:
            if name is None: pass
            elif name == '/': pass
            elif parent is None:
                xxx = Node.objects.create(
                    name= name, 
                    fullname= server + path,
                    kind= kind,
                    status= status)
            else: 
                xxx = Node.objects.create(
                    name=name,
                    fullname= server + path,
                    kind= kind,
                    status= status,
                    parent=parent)
        else: xxx = node

        add(path, kind)

        defstatus = node.get_defstatus()
        if defstatus != ecflow.DState.queued: 
            add(path, "defstatus", str(defstatus))
            
        autocancel = node.get_autocancel()
        if autocancel: add(path, "autocancel", str(autocancel))
        
        repeat = node.get_repeat()
        if not repeat.empty(): add(path, "repeat", str(repeat))
    
        late = node.get_late()
        if late: add(path, "", str(late))

        complete = node.get_complete()
        if complete:
            add(path, "complete", str(complete))

        trigger = node.get_trigger()
        if trigger:
            add(path, "trigger", str(trigger))
            
        for item in node.variables:add(path, "edit", "%s %s" % (item.name(), item.value()))
        for item in node.meters:   add(path, "", "%s" % item)
        for item in node.events:   add(path, "event", item.name())
        for item in node.labels:   add(path, "label", item.name())
        for item in node.limits:   add(path, "", "%s" % item)
        for item in node.inlimits: add(path, "inlimit", item.name())
        for item in node.times:    add(path, "", "%s" % item)
        for item in node.todays:   add(path, "", "%s" % item)
        for item in node.dates:    add(path, "", "%s" % item)
        for item in node.days:     add(path, "", "%s" % item)
        for item in node.crons:    add(path, "", "%s" % item)
        return xxx

class DjangoEcflowClient(object):
    def __init__(self, host_port="localhost:31415"):

        host_port= "vsms1:32222"
        self.host = host_port.split(":")[0]
        self.port = host_port.split(":")[1]
        self.clt = ecflow.Client(host_port)
        self.server = host_port

        self.clt.ch_register( False, [ "fsobs", ] )


    def update(self):
        if not self.clt: return
        if not self.clt.news_local(): return
        self.clt.sync_local() 
        try:    
            for path in self.clt.changed_node_paths:
                print "   changed node path " + path;
                if path == '/' :
                    print "Root node/server was updated "
                else: 
                    node = self.clt.get_defs().find_abs_node(path)
                    process_nc(node, None, server)
        except:
            node = self.clt.get_defs().find_abs_node('/')
            if node: process_nc(node, None, self.server + ':')


class Servers(models.Model):
    id = models.AutoField(primary_key=True)
    host = models.CharField(max_length=10, blank=True)
    port = models.IntegerField()
    kind = models.CharField(max_length=10, blank=True)

class Tasks(models.Model):
    name = models.CharField(max_length=20, primary_key=True)

class Task(models.Model):
    name = models.CharField(max_length=384, primary_key=True)
    nick = models.ForeignKey(Tasks,db_column='nick',to_field='name')
    server = models.ForeignKey(Servers,db_column='server',to_field='id')
    desc = models.CharField(max_length=384)

class Events(models.Model):
    name = models.CharField(max_length=20, primary_key=True)

class Event(models.Model):
    id = models.AutoField(primary_key=True)
    kind = models.ForeignKey(Events,db_column='kind',to_field='name')
    task = models.ForeignKey(Task,db_column='task',to_field='name')
    date = models.DateTimeField()

class Streams(models.Model):
    name = models.CharField(max_length=4, primary_key=True)

class Expvers(models.Model):
    name = models.CharField(max_length=4, primary_key=True)

class Dataset(models.Model):
    id = models.AutoField(primary_key=True)
    klass = models.CharField(max_length=2)
    domain = models.CharField(max_length=1)
    stream = models.ForeignKey(Streams,db_column='stream',to_field='name')
    expver = models.ForeignKey(Expvers,db_column='expver',to_field='name')
    path   = models.CharField(max_length=384)
    date = models.DateTimeField()

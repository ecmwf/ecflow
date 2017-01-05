//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InputEventLog.hpp"

#include "DirectoryHandler.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDebug>
#include <QEvent>
#include <QFile>
#include <QMenu>
#include <QMouseEvent>
#include <QTabBar>

#include "TimeStamp.hpp"

static QString objectPath(QObject *obj)
{
    QString res;
    for(; obj; obj = obj->parent())
    {
        if (!res.isEmpty())
            res.prepend("/");
        QString s=obj->objectName();
        //QString cn(obj->metaObject()->className());
        if(s.isEmpty()) s="?";
        //res.prepend("[" + cn + "]" +s);
        res.prepend(s);
    }
    return res;
}

InputEventLog::InputEventLog(QObject* parent) : QObject(parent)
{
    outFile_=new QFile(QString::fromStdString(DirectoryHandler::uiLogFileName()));
    if(outFile_->open(QFile::WriteOnly | QFile::Truncate))
    {
        out_.setDevice(outFile_);
    }
}

InputEventLog::~InputEventLog()
{
    outFile_->close();
    delete outFile_;
}

bool InputEventLog::eventFilter(QObject *obj, QEvent *event)
{
    if(out_.device())
    {
        //out_ << event->type() << " " << obj->objectName() << " " << obj->metaObject()->className() << "\n";
        //out_.flush();

        if(event->type() == QEvent::MouseButtonPress)
        {
            logMousePress(obj,static_cast<QMouseEvent*>(event));
        }
        else if(event->type() == QEvent::MouseButtonRelease)
        {
            logMouseRelease(obj,static_cast<QMouseEvent*>(event));
        }
        else if(event->type() == QEvent::Close)
        {
            logClose(obj,static_cast<QCloseEvent*>(event));
        }
        else if(event->type() == QEvent::ContextMenu)
        {
            logContextMenu(obj,static_cast<QContextMenuEvent*>(event));
        }
    }

    return QObject::eventFilter(obj,event);
}

void InputEventLog::logMousePress(QObject* obj,QMouseEvent *event)
{
    QString cn(obj->metaObject()->className());
    if(cn != "QWidgetWindow" && cn != "QMenuBar" &&
       cn != "QToolBar" && cn != "MainWindow" && cn != "QScrollBar" )
    {
        std::string s;
        ecf::TimeStamp::now_in_brief(s);
        out_ << s.c_str() << "mp " << cn << " " << objectPath(obj);

        if(cn == "QTabBar")
        {
            if(QTabBar* t=static_cast<QTabBar*>(obj))
            {
                int idx=t->tabAt(event->pos());
                if(idx >=0)
                {
                    out_ << " tab=" << t->tabText(idx);
                }
            }
        }
        else if(cn == "QMenu")
        {
            if(QMenu* m=static_cast<QMenu*>(obj))
            {
                if(QAction* ac=m->actionAt(event->pos()))
                {
                    out_ << " ac=" << ac->objectName();
                }
            }
        }
        out_ << "\n";
        out_.flush();
    }
}

void InputEventLog::logMouseRelease(QObject* obj,QMouseEvent *event)
{
    QString cn(obj->metaObject()->className());
    if(cn == "QMenu")
    {
        std::string s;
        ecf::TimeStamp::now_in_brief(s);
        out_ << s.c_str() << "mr " << cn << " " << objectPath(obj);
        if(QMenu* m=static_cast<QMenu*>(obj))
        {
            if(QAction* ac=m->actionAt(event->pos()))
            {
                out_ << " ac=" << ac->objectName();
            }
        }
        out_ << "\n";
        out_.flush();
    }
}

void InputEventLog::logClose(QObject* obj,QCloseEvent *event)
{
    QString cn(obj->metaObject()->className());
    if(cn != "QWidgetWindow" && cn != "QTipLabel")
    {
        std::string s;
        ecf::TimeStamp::now_in_brief(s);
        out_ << s.c_str() << "cl " << cn << " " << objectPath(obj) << "\n";
        out_.flush();
    }
}

void InputEventLog::logContextMenu(QObject* obj,QContextMenuEvent *event)
{
    QString cn(obj->metaObject()->className());
    if(cn != "QWidgetWindow")
    {
        std::string s;
        ecf::TimeStamp::now_in_brief(s);
        out_ << s.c_str() << "cm " << cn << " " << objectPath(obj) << "\n";
        out_.flush();
    }
}

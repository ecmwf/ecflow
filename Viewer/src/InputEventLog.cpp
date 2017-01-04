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
        if(s.isEmpty()) s="?";
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

        if(event->type() == QEvent::MouseButtonRelease)
        {
            mouseRelease(obj,static_cast<QMouseEvent*>(event));

            //out_ << "mc " << obj->metaObject()->className() << " " << obj->objectName() << " " << objectPath(obj) << "\n";
           //out_.flush();
        }
    }

    return QObject::eventFilter(obj,event);
}

void InputEventLog::mouseRelease(QObject* obj,QMouseEvent *event)
{
    QString cn(obj->metaObject()->className());
    if(cn != "QWidget" && cn != "QWidgetWindow" && cn != "QMenuBar" &&
       cn != "QToolBar" && cn != "MainWindow")
    {
        std::string s;
        ecf::TimeStamp::now_in_brief(s);
        out_ << s.c_str() << "mr " << cn << " " << objectPath(obj);

        if(cn == "QTabBar")
        {
            if(QTabBar* t=static_cast<QTabBar*>(obj))
            {
                int idx=t->tabAt(event->pos());
                if(idx >=0)
                {
                    out_ << " " << t->tabText(idx);
                }
            }
        }
        else if(cn == "QMenu")
        {
            if(QMenu* m=static_cast<QMenu*>(obj))
            {
                if(QAction* ac=m->actionAt(event->pos()))
                {
                    out_ << " " << ac->objectName();
                }
            }
        }
        out_ << "\n";
        out_.flush();
    }
}

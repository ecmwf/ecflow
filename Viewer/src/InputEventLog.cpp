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

#include <QDebug>
#include <QEvent>
#include <QFile>

static QString objectPath(QObject *obj)
{
    QString res;
    for(; obj; obj = obj->parent())
    {
        if (!res.isEmpty())
            res.prepend("/");
        res.prepend(obj->objectName());
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
        if(event->type() == QEvent::MouseButtonPress)
        {
           out_ << "mouseClick " << obj->metaObject()->className() << objectPath(obj) << "\n";
           out_.flush();
        }
    }

    return QObject::eventFilter(obj,event);
}

#if 0
    if (clonedEv)
    {
        int timeOffset;
        QDateTime curDt(QDateTime::currentDateTime());
        timeOffset = m_RecordingStartTime.daysTo(curDt) * 24 * 3600 * 1000 + m_RecordingStartTime.time().msecsTo(curDt.time());
        m_Recording.push_back(EventDelivery(timeOffset, obj, clonedEv));
    }

    return false;
}

#endif


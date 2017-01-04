//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INPUTEVENTLOG_HPP
#define INPUTEVENTLOG_HPP

#include <QObject>
#include <QTextStream>

class QFile;
class QMouseEvent;

class InputEventLog : public QObject
{
public:
    InputEventLog(QObject* parent);
    ~InputEventLog();

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void mouseRelease(QObject* obj,QMouseEvent *e);

    QFile *outFile_;
    QTextStream out_;
};


#endif // INPUTEVENTLOG_HPP


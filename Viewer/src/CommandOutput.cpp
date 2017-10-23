//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CommandOutput.hpp"

#include "CommandOutputDialog.hpp"

CommandOutputHandler* CommandOutputHandler::instance_=0;

//===============================================
//
// CommandOutput
//
//==============================================

CommandOutput::CommandOutput(QString cmd,QString cmdDef,QDateTime runTime) :
    enabled_(true), command_(cmd), commandDef_(cmdDef),
    runTime_(runTime), status_(RunningStatus)
{

}

void CommandOutput::appendOutput(QString txt,int maxSize,bool& trimmed)
{
    output_+=txt;
    trimmed=false;
    if(output_.size() > maxSize)
    {
        output_=output_.right(maxSize-100);
        trimmed=true;
    }
}

void CommandOutput::appendError(QString txt,int maxSize,bool& trimmed)
{
    error_+=txt;
    trimmed=false;
    if(error_.size() > maxSize)
    {
        error_=error_.right(maxSize-100);
        trimmed=true;
    }
}

QString CommandOutput::statusStr() const
{
    static QString finishedStr("finished");
    static QString failedStr("failed");
    static QString runningStr("running");

    switch(status_)
    {
    case FinishedStatus:
        return finishedStr;
    case FailedStatus:
        return failedStr;
    case RunningStatus:
        return runningStr;
    default:
        return QString();
    }

    return QString();
}

QColor CommandOutput::statusColour() const
{
    static QColor redColour(255,0,0);
    static QColor greenColour(9,160,63);
    static QColor blackColour(0,0,0);

    switch(status_)
    {
    case CommandOutput::FinishedStatus:
        return blackColour;
    case CommandOutput::FailedStatus:
        return redColour;
    case CommandOutput::RunningStatus:
        return greenColour;
    default:
        return blackColour;
    }

    return blackColour;
}

//===============================================
//
// CommandOutputHandler
//
//===============================================

CommandOutputHandler::CommandOutputHandler(QObject* parent) :
    QObject(parent),
    maxNum_(25),
    maxOutputSize_(1000000),
    maxErrorSize_(30000)
{

}

CommandOutputHandler* CommandOutputHandler::instance()
{
    if(!instance_)
        instance_=new CommandOutputHandler(0);

    return instance_;
}

int CommandOutputHandler::indexOfItem(CommandOutput_ptr item) const
{
    if(!item)
        return -1;

    for(int i=0; i < items_.count(); i++)
    {
        if(item.get() == items_[i].get())
            return i;
    }

    return -1;
}

void CommandOutputHandler::appendOutput(CommandOutput_ptr item,QString txt)
{
    if(item)
    {
        bool trimmed=false;
        item->appendOutput(txt,maxOutputSize_,trimmed);
        CommandOutputDialog::showDialog();
        if(trimmed==false)
            Q_EMIT itemOutputAppend(item,txt);
        else
            Q_EMIT itemOutputReload(item);
    }
}

void CommandOutputHandler::appendError(CommandOutput_ptr item,QString txt)
{
    if(item)
    {
        bool trimmed=false;
        item->appendError(txt,maxErrorSize_,trimmed);
        CommandOutputDialog::showDialog();
        if(trimmed==false)
            Q_EMIT itemErrorAppend(item,txt);
        else
            Q_EMIT itemErrorReload(item);
    }
}

void CommandOutputHandler::finished(CommandOutput_ptr item)
{
    if(item)
    {
        item->setStatus(CommandOutput::FinishedStatus);
        Q_EMIT itemStatusChanged(item);
    }
}

void CommandOutputHandler::failed(CommandOutput_ptr item)
{
    if(item)
    {
        item->setStatus(CommandOutput::FailedStatus);
        Q_EMIT itemStatusChanged(item);
    }
}

CommandOutput_ptr CommandOutputHandler::addItem(QString cmd,QString cmdDef,QDateTime runTime)
{
    CommandOutputDialog::showDialog();
    CommandOutput_ptr item=
            CommandOutput_ptr(new CommandOutput(cmd,cmdDef,runTime));
    Q_EMIT itemAddBegin();
    items_ << item;
    checkItems();
    Q_EMIT itemAddEnd();
    return item;
}

void CommandOutputHandler::checkItems()
{
    Q_ASSERT(maxNum_ >0);
    while(items_.count() > maxNum_)
    {
        Q_ASSERT(items_.count() > 0);
        CommandOutput_ptr item=items_.first();
        item->setEnabled(false);
        items_.remove(0);
    }
}

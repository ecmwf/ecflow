//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "WmWorkspaceHandler.hpp"

#include <QtGlobal>
#include <QDebug>
#include <QWidget>
#include <QProcess>

#include "UiLog.hpp"

int WmWorkspaceHandler::commandTested_=0;

#define _UI_WMWORKSPACEHANDLER_DEBUG

bool WmWorkspaceHandler::switchTo(QWidget* sourceWidget,QWidget* targetWidget)
{
#ifdef _UI_WMWORKSPACEHANDLER_DEBUG
    UI_FUNCTION_LOG
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if(sourceWidget == 0 || targetWidget ==0)
        return false;

    if(!hasCommand())
        return false;

    Q_ASSERT(sourceWidget);
    Q_ASSERT(targetWidget);

    QWidget* sourceWin=sourceWidget->window();
    QWidget* targetWin=sourceWidget->window();
    if(sourceWin && targetWin)
    {
        //Get the window ids
        int sourceWinId=sourceWidget->winId();
        int targetWinId=targetWidget->winId();

#ifdef _UI_WMWORKSPACEHANDLER_DEBUG
        UiLog().dbg() << " sourceWinId=" << sourceWinId  << " targetWinId=" << targetWinId;
#endif
        if(sourceWinId != targetWinId)
        {
            //Get the virtual workspace id
            int sourceWsId=workspaceId(sourceWinId);
            int targetWsId=workspaceId(targetWinId);
#ifdef _UI_WMWORKSPACEHANDLER_DEBUG
            UiLog().dbg() << " sourceWsId=" << sourceWsId  << " targetWsId=" << targetWsId;
#endif
            //move the source window to the target workspace
            //and switch to this workspace
            if(sourceWsId >=0 && targetWsId >=0 &&
               sourceWsId !=targetWsId)
            {
                moveAndSwitchToWorkspace(sourceWinId,targetWsId);
                return true;
            }
        }
   }
#endif

    return false;
}

bool WmWorkspaceHandler::hasCommand()
{
    if(commandTested_ == 0)
    {
        commandTested_=2;

        QProcess proc;
        proc.start("xdotool",QStringList() << "-v");
        proc.waitForFinished(3000);

        if(proc.exitStatus() == QProcess::NormalExit)
            commandTested_=1;
    }

    Q_ASSERT(commandTested_ > 0);
    return (commandTested_== 1);
}

int WmWorkspaceHandler::workspaceId(int winId)
{
    QProcess proc;
    proc.start("xdotool",
               QStringList() << "get_desktop_for_window" << QString::number(winId));

    proc.waitForFinished(3000);

    if(proc.exitStatus() == QProcess::NormalExit)
    {
        QString result(proc.readAllStandardOutput());
        QRegExp re("^\\d+\\n?$");
        if(re.exactMatch(result))
            return result.toInt();
        else
        {
            UiLog().err() << "Failed to get workspace id for window=" << winId << " using command " << proc.program();
            UiLog().err() << " error: invalid id=" << result << "returned";
        }
    }
    else
    {
        UiLog().err() << "Failed to get workspace id for window=" << winId << " using command " << proc.program();
        UiLog().err() << " error:" << proc.program() << QString(proc.readAllStandardError());
    }

    return -1;
}

void WmWorkspaceHandler::moveAndSwitchToWorkspace(int winId,int wsId)
{
    QProcess proc;
    proc.start("xdotool",QStringList() << "set_desktop_for_window" <<
               QString::number(winId) << QString::number(wsId));
    proc.waitForFinished(3000);

    if(proc.exitStatus() == QProcess::NormalExit)
    {
        QProcess procSwitch;
        procSwitch.start("xdotool",QStringList() << "set_desktop" << QString::number(wsId));
        procSwitch.waitForFinished(3000);
    }
}
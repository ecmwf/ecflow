//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef WMWORKSPACEHANDLER_HPP
#define WMWORKSPACEHANDLER_HPP

class QWidget;

class WmWorkspaceHandler
{
public:
    static bool switchTo(QWidget* sourceWidget,QWidget* targetWidget);
private:
    static bool hasCommand();
    static int workspaceId(int winId);
    static void moveAndSwitchToWorkspace(int winId,int wsId);

    static int commandTested_;
};

#endif // WMWORKSPACEHANDLER_HPP

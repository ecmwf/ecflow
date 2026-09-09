/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_WmWorkspaceHandler_HPP
#define ecflow_viewer_WmWorkspaceHandler_HPP

class QWidget;

class WmWorkspaceHandler {
public:
    static bool switchTo(QWidget* sourceWidget, QWidget* targetWidget);

private:
    static bool hasCommand();
    static int workspaceId(int winId);
    static void moveAndSwitchToWorkspace(int winId, int wsId);

    static int commandTested_;
};

#endif /* ecflow_viewer_WmWorkspaceHandler_HPP */

// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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

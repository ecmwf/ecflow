// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QMainWindow>

#include "ServerFilter.hpp"
#include "ui_NodeSearchWindow.h"

class ServerFilter;

class NodeSearchWindow : public QMainWindow, protected Ui::NodeSearchWindow {
    Q_OBJECT

public:
    explicit NodeSearchWindow(QWidget* parent = nullptr);
    ~NodeSearchWindow() override;

    NodeSearchWidget* queryWidget() const;

protected Q_SLOTS:
    void closeIt();
    void slotOwnerDelete();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void readSettings();
    void writeSettings();
};

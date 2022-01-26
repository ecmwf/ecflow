//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef NODESEARCHWINDOW_HPP_
#define NODESEARCHWINDOW_HPP_

#include <QMainWindow>

#include "ServerFilter.hpp"

#include "ui_NodeSearchWindow.h"

class ServerFilter;

class NodeSearchWindow : public QMainWindow, protected Ui::NodeSearchWindow
{
    Q_OBJECT

public:
    explicit NodeSearchWindow(QWidget *parent = nullptr);
    ~NodeSearchWindow() override;

    NodeSearchWidget* queryWidget() const;

protected Q_SLOTS:
    void closeIt();
    void slotOwnerDelete();

protected:
	void closeEvent(QCloseEvent * event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void readSettings();
    void writeSettings();
};


#endif

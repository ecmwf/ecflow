//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef NODESEARCHDIALOG_HPP_
#define NODESEARCHDIALOG_HPP_

#include <QDialog>

#include "ServerFilter.hpp"

#include "ui_NodeSearchDialog.h"

class ServerFilter;

class NodeSearchDialog : public QDialog, protected Ui::NodeSearchDialog
{
    Q_OBJECT

public:
    explicit NodeSearchDialog(QWidget *parent = nullptr);
    ~NodeSearchDialog();

    NodeSearchWidget* queryWidget() const;

protected Q_SLOTS:
	void accept();
    void reject();
    void slotOwnerDelete();

protected:
	void closeEvent(QCloseEvent * event);

private:
    void readSettings();
    void writeSettings();
};


#endif

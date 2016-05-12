//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEFILTERDIALOG_HPP_
#define VIEWER_SRC_NODEFILTERDIALOG_HPP_

#include <QDialog>

#include "ui_NodeFilterDialog.h"

class ServerFilter;

class NodeFilterDialog : public QDialog, protected Ui::NodeFilterDialog
{
    Q_OBJECT

public:
    explicit NodeFilterDialog(QWidget *parent = 0);
    ~NodeFilterDialog();

    void setQuery(NodeQuery*);
    NodeQuery* query() const;
    void setServerFilter(ServerFilter*);

protected Q_SLOTS:
	void accept();
    void reject();

protected:
	void closeEvent(QCloseEvent * event);

private:
    void readSettings();
    void writeSettings();
};



#endif /* VIEWER_SRC_NODEFILTERDIALOG_HPP_ */

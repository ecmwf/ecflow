//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLISTSYNCDIALOG_HPP
#define SERVERLISTSYNCDIALOG_HPP

#include <QDialog>

#include <vector>

#include "ui_ServerListSyncDialog.h"

class ServerListSyncConflictItem;


class ServerListSyncDialog : public QDialog, protected Ui::ServerListSyncDialog
{
    Q_OBJECT

public:
    explicit ServerListSyncDialog(QWidget *parent = 0);
    ~ServerListSyncDialog();

    void init(const std::vector<ServerListSyncConflictItem*>&);

protected Q_SLOTS:
//	void accept();
//    void reject();
//    void slotOwnerDelete();
    void slotConflictSelected(int row);

protected:
    //void closeEvent(QCloseEvent * event);

private:
    QString buildTable(QString name,QString host,QString port) const;
    //void readSettings();
    //void writeSettings();

    std::vector<ServerListSyncConflictItem*> items_;
};


#endif // SERVERLISTSYNCDIALOG_HPP

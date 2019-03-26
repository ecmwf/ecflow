//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ADDLABELDIALOG_HPP
#define ADDLABELDIALOG_HPP

#include <QDialog>

#include "VInfo.hpp"

class QCloseEvent;

namespace Ui {
    class AddLabelDialog;
}

class AddLabelDialog : public QDialog
{
    Q_OBJECT

public:
    AddLabelDialog(VInfo_ptr info, QString labelName, QWidget* parent=0);

protected Q_SLOTS:
    void accept();
    void reject();

protected:
    void closeEvent(QCloseEvent * event);
    void writeSettings();
    void readSettings();

    Ui::AddLabelDialog* ui_;
    VInfo_ptr info_;
};

#endif // ADDLABELDIALOG_HPP

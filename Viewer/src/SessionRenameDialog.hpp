//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SESSIONRENAMEDIALOG_HPP
#define SESSIONRENAMEDIALOG_HPP

#include <QDialog>
#include "ui_SessionRenameDialog.h"

#include "SessionHandler.hpp"

namespace Ui {
class SessionRenameDialog;
}

class SessionRenameDialog : public QDialog, protected Ui::SessionRenameDialog
{
    Q_OBJECT

public:
    explicit SessionRenameDialog(QWidget *parent = 0);
    ~SessionRenameDialog();

	std::string newName() {return newName_;};

public Q_SLOTS:
	void on_buttonBox__accepted();
	void on_buttonBox__rejected();

private:
	std::string newName_;
 };

#endif // SESSIONRENAMEDIALOG_HPP

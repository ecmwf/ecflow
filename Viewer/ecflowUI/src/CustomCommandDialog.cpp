/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CustomCommandDialog.hpp"

CustomCommandDialog::CustomCommandDialog(QWidget* /*parent*/) {
    setupUi(this);

    // when the user clicks the 'Run' button, we close the dialog with ACCEPT
    connect(commandDesigner_->runButton(), SIGNAL(clicked()), this, SLOT(accept()));
}

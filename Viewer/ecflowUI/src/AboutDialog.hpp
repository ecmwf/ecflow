/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_AboutDialog_HPP
#define ecflow_viewer_AboutDialog_HPP

#include <QDialog>

#include "ui_AboutDialog.h"

class AboutDialog : public QDialog, protected Ui::AboutDialog {
public:
    explicit AboutDialog(QWidget* parent = nullptr);
};

#endif /* ecflow_viewer_AboutDialog_HPP */

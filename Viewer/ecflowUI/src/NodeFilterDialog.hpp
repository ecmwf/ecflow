/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeFilterDialog_HPP
#define ecflow_viewer_NodeFilterDialog_HPP

#include <QDialog>

#include "ui_NodeFilterDialog.h"

class ServerFilter;

class NodeFilterDialog : public QDialog, protected Ui::NodeFilterDialog {
    Q_OBJECT

public:
    explicit NodeFilterDialog(QWidget* parent = nullptr);
    ~NodeFilterDialog() override;

    void setQuery(NodeQuery*);
    NodeQuery* query() const;
    void setServerFilter(ServerFilter*);

protected Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void readSettings();
    void writeSettings();
};

#endif /* ecflow_viewer_NodeFilterDialog_HPP */

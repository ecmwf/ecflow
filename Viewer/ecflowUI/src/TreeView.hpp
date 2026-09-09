/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TreeView_HPP
#define ecflow_viewer_TreeView_HPP

#include <QTreeView>

class TreeView : public QTreeView {
public:
    explicit TreeView(QWidget* parent = nullptr);
};

#endif /* ecflow_viewer_TreeView_HPP */

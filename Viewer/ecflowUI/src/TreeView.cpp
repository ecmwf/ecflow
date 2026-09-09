/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TreeView.hpp"

#include <QPalette>

TreeView::TreeView(QWidget* parent)
    : QTreeView(parent) {
    //!!!!We need to do it because:
    // The background colour between the views left border and the nodes cannot be
    // controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    // colour from the palette. Here we set this to transparent so that Qt could leave
    // this are empty and we will fill it appropriately in our delegate.

    QPalette pal = palette();
    pal.setColor(QPalette::Highlight, QColor(255, 255, 255, 0));
    setPalette(pal);
}

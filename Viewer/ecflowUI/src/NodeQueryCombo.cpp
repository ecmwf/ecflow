/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "NodeQueryCombo.hpp"

#include <QVariant>

#include "NodeQuery.hpp"
#include "NodeQueryHandler.hpp"

NodeQueryCombo::NodeQueryCombo(QWidget* parent)
    : QComboBox(parent) {
    for (auto it : NodeQueryHandler::instance()->items()) {
        addItem(QString::fromStdString(it->name()));
    }

    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentChanged(int)));
}

void NodeQueryCombo::slotCurrentChanged(int current) {
    if (current != -1) {
        Q_EMIT changed(itemData(current).toString());
    }
}

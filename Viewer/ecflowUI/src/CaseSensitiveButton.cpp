/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CaseSensitiveButton.hpp"

CaseSensitiveButton::CaseSensitiveButton(QWidget* parent)
    : QToolButton(parent) {
    connect(this, SIGNAL(clicked(bool)), this, SLOT(slotClicked(bool)));

    setCheckable(true);
    setIcon(QPixmap(":/viewer/case_sensitive.svg"));

    tooltip_[true]  = tr("Case sensitivity is <b>on</b>. Toggle to turn it off.");
    tooltip_[false] = tr("Case sensitivity is <b>off</b>. Toggle to turn it on.");

    setToolTip(tooltip_[false]);
}

void CaseSensitiveButton::slotClicked(bool b) {
    setToolTip(tooltip_[b]);
    Q_EMIT changed(b);
}

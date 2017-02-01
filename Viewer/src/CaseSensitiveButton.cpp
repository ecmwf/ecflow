//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CaseSensitiveButton.hpp"

CaseSensitiveButton::CaseSensitiveButton(QWidget *parent) : QToolButton(parent)
{
	connect(this,SIGNAL(clicked(bool)),
			this,SLOT(slotClicked(bool)));

	setCheckable(true);
	setIcon(QPixmap(":/viewer/case_sensitive.svg"));

	tooltip_[true]=tr("Case sensitivity is <b>on</b>. Toggle to turn it off.");
	tooltip_[false]=tr("Case sensitivity is <b>off</b>. Toggle to turn it on.");

	setToolTip(tooltip_[false]);
}

void CaseSensitiveButton::slotClicked(bool b)
{
	setToolTip(tooltip_[b]);
	Q_EMIT changed(b);
}


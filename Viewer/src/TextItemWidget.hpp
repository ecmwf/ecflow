//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TEXTITEMWIDGET_HPP_
#define TEXTITEMWIDGET_HPP_

#include <QWidget>

#include "ui_TextItemWidget.h"

class TextItemWidget : public QWidget, protected Ui::TextItemWidget
{
public:
	explicit TextItemWidget(QWidget *parent=0);
};

#endif


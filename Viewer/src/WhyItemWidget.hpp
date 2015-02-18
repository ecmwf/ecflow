//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef WHYITEMWIDGET_HPP_
#define WHYITEMWIDGET_HPP_

#include <QPlainTextEdit>

#include "InfoPanelItem.hpp"
#include "TextItemWidget.hpp"
#include "VInfo.hpp"

class WhyItemWidget : public TextItemWidget, public InfoPanelItem
{
public:
	WhyItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	void nodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&) {};

private:
	QString why(Node* n) const;


};

#endif


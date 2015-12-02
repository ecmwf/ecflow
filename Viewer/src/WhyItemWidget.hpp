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
#include "CodeItemWidget.hpp"
#include "VInfo.hpp"

class VNode;

class WhyItemWidget : public CodeItemWidget, public InfoPanelItem
{
public:
	explicit WhyItemWidget(QWidget *parent=0);
	~WhyItemWidget();

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

	void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {};
	void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

protected:
	void updateWidgetState() {};

private:
	QString why() const;
};

#endif


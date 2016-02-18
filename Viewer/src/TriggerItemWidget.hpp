//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TRIGGERITEMWIDGET_HPP_
#define TRIGGERITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

#include "ui_TriggerItemWidget.h"

class TriggerItemWidget : public QWidget, public InfoPanelItem, protected Ui::TriggerItemWidget
{
public:
	explicit TriggerItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
    void clearContents();
    void becameSelected() {}
    void becameUnselected() {}

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {}

protected:
    void updateWidgetState() {}

};

#endif


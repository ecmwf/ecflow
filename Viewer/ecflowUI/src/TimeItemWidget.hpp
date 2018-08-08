//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TIMEITEMWIDGET_HPP_
#define TIMEITEMWIDGET_HPP_
#include "ui_TimeItemWidget.h"

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class LineEdit;

class TimeItemWidget 
  : public QWidget
  , public InfoPanelItem
  , protected Ui::TimeItemWidget
		       
{
public:
	explicit TimeItemWidget(QWidget *parent=nullptr);

	void reload(VInfo_ptr) override;
	QWidget* realWidget() override;
    void clearContents() override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    QGraphicsView* view() { return graphicsView; }

protected:
   void updateState(const ChangeFlags&) override {}
};

#endif


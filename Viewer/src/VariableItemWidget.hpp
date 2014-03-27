//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VARIABLEITEMWIDGET_HPP_
#define VARIABLEITEMWIDGET_HPP_

#include <QTreeView>

#include "InfoPanelItem.hpp"
#include "ViewNodeInfo.hpp"

class VariableModel;

class VariableView : public QTreeView
{
Q_OBJECT

public:
		VariableView(QWidget *parent=0);
		//void reload();
		void reload(ViewNodeInfo_ptr);

public slots:
	void slotSelectItem(const QModelIndex&);

protected:
	VariableModel *model_;
};


class VariableItemWidget : public QWidget, public InfoPanelItem
{
public:
	VariableItemWidget(QWidget *parent=0);

	void reload(ViewNodeInfo_ptr);
	QWidget* realWidget();


protected:
	VariableView *view_;

};

#endif


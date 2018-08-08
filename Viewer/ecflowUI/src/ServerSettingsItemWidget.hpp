//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERSETTINGSITEMWIDGET_HPP_
#define SERVERSETTINGSITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "PropertyEditor.hpp"
#include "VInfo.hpp"

#include "ui_ServerSettingsItemWidget.h"

class QAbstractButton;

class VNode;

class ServerSettingsItemWidget : public QWidget, public InfoPanelItem, protected Ui::ServerSettingsItemWidget
{
Q_OBJECT

public:
	explicit ServerSettingsItemWidget(QWidget *parent=nullptr);

	void reload(VInfo_ptr);
	QWidget* realWidget();
    void clearContents();

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {}

protected Q_SLOTS:
	void slotClicked(QAbstractButton* button);
	void slotEditorChanged();

protected:
    void updateState(const ChangeFlags&);
};

#endif


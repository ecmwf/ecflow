//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef EDITITEMWIDGET_HPP_
#define EDITITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

#include "ui_EditItemWidget.h"

class PropertyMapper;

class EditItemWidget : public QWidget, public InfoPanelItem, protected Ui::EditItemWidget
{
Q_OBJECT

public:
	explicit EditItemWidget(QWidget *parent=0);

	void reload(VInfo_ptr);
	QWidget* realWidget();
    void clearContents();

	//From VInfoPresenter
	void infoReady(VReply*);
	void infoFailed(VReply*);
	void infoProgress(VReply*);

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {}

protected Q_SLOTS:
	void on_preprocTb__toggled(bool);
	void on_submitTb__clicked(bool);
	void on_searchTb__clicked();
	void on_gotoLineTb__clicked();
	void on_fontSizeUpTb__clicked();
	void on_fontSizeDownTb__clicked();

protected:
	bool preproc() const;
	bool alias() const;
    void updateState(const ChangeFlags&) {}

	bool preproc_;
	bool alias_;
};

#endif


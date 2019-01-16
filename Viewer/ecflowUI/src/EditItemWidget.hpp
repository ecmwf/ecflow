//============================================================================
// Copyright 2009-2019 ECMWF.
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

class EditItemWidget : public QWidget, public InfoPanelItem, protected Ui::EditItemWidget
{
Q_OBJECT

public:
	explicit EditItemWidget(QWidget *parent=nullptr);

	void reload(VInfo_ptr) override;
	QWidget* realWidget() override;
    void clearContents() override;

	//From VInfoPresenter
	void infoReady(VReply*) override;
	void infoFailed(VReply*) override;
	void infoProgress(VReply*) override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

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
    void updateState(const ChangeFlags&) override {}
};

#endif


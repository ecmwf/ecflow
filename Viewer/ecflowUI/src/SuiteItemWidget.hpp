//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SUITEITEMWIDGET_HPP_
#define SUITEITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"
#include "SuiteFilterObserver.hpp"

#include "ui_SuiteItemWidget.h"

class SuiteModel;

class SuiteItemWidget : public QWidget, public InfoPanelItem, public SuiteFilterObserver, protected Ui::SuiteItemWidget
{
Q_OBJECT

public:
	explicit SuiteItemWidget(QWidget *parent=nullptr);

	void reload(VInfo_ptr) override;
	QWidget* realWidget() override;
    void clearContents() override;  

	void infoReady(VReply*) override;
	void infoFailed(VReply*) override;
    void infoProgress(VReply*) override {}

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

    void notifyChange(SuiteFilter *filter) override;
    void notifyDelete(SuiteFilter *filter) override;

protected Q_SLOTS:
	void on_autoCb_clicked(bool);
	void on_enableTb_clicked(bool);
	void on_selectAllTb_clicked(bool);
	void on_unselectAllTb_clicked(bool);
	void on_syncTb_clicked(bool);
    void on_okTb_clicked(bool);
    void on_resetTb_clicked(bool);
    void on_removeTb_clicked(bool);
	void slotModelEdited(const QModelIndex&,const QModelIndex&);
    void slotModelDataUpdated();

protected:
	void updateData();    
    void updateState(const ChangeFlags&) override;
    void checkActionState();

    SuiteModel *model_;
    bool columnsAdjusted_{false};
    bool edited_{false};
};

#endif


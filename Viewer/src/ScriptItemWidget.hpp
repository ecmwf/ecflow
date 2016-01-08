//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SCRIPTITEMWIDGET_HPP_
#define SCRIPTITEMWIDGET_HPP_

#include "InfoPanelItem.hpp"
#include "CodeItemWidget.hpp"

class ScriptItemWidget : public CodeItemWidget, public InfoPanelItem
{
public:
	explicit ScriptItemWidget(QWidget *parent=0);
	~ScriptItemWidget();

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();
    
    //From VInfoPresenter
    void infoReady(VReply*);
    void infoFailed(VReply*);
    void infoProgress(VReply*);

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {};

protected:
    void updateWidgetState() {};
};

#endif


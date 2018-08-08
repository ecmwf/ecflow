//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "HtmlItemWidget.hpp"
#include "VInfo.hpp"

class VNode;

class WhyItemWidget : public HtmlItemWidget, public InfoPanelItem
{
Q_OBJECT

public:
	explicit WhyItemWidget(QWidget *parent=nullptr);
	~WhyItemWidget();

	void reload(VInfo_ptr);
	QWidget* realWidget();
    void clearContents();

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {}

protected Q_SLOTS:
    void anchorClicked(const QUrl& link);

protected:
    void updateState(const ChangeFlags&);
    void serverSyncFinished();

private:
    void load();
    QString why() const;
    QString makeHtml(const std::vector<std::string>&,const std::vector<std::string>&) const;
    QString makeHtml(const std::vector<std::string>&) const;

    QMap<QString,QString> stateMap_;
};

#endif


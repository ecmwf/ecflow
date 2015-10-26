//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_ATTRIBUTESEARCHPANEL_HPP_
#define VIEWER_SRC_ATTRIBUTESEARCHPANEL_HPP_

#include <QWidget>

#include "ui_AttributeSearchPanel.h"
#include "ui_EventSearchWidget.h"
#include "ui_LabelSearchWidget.h"

class QStandardItemModel;

class AttributeSearchWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AttributeSearchWidget(QString, QWidget* parent=0);
	virtual QString query()=0;
	QString labelName() const {return labelName_;}

protected Q_SLOTS:
	void buildQuery() {};

Q_SIGNALS:
	void queryChanged();

protected:
	QString labelName_;
};


class EventSearchWidget : public AttributeSearchWidget, protected Ui::EventSearchWidget
{
    Q_OBJECT

public:
    explicit EventSearchWidget(QWidget *parent = 0);
    ~EventSearchWidget() {};
    QString query();

protected Q_SLOTS:
    void slotTextChanged(QString);
};

class LabelSearchWidget : public AttributeSearchWidget, protected Ui::LabelSearchWidget
{
    Q_OBJECT

public:
    explicit LabelSearchWidget(QWidget *parent = 0);
    ~LabelSearchWidget() {};
    QString query();

protected Q_SLOTS:
    void slotTextChanged(QString);
};

class AttributeSearchPanel : public QWidget, protected Ui::AttributeSearchPanel
{
    Q_OBJECT

public:
    explicit AttributeSearchPanel(QWidget *parent = 0);
    ~AttributeSearchPanel() {};
    QString query() const {return query_;}

public Q_SLOTS:
    void buildQuery();

protected Q_SLOTS:
	void slotChangePage(const QModelIndex& idx);

Q_SIGNALS:
	void queryChanged();

private:
	void addPage(AttributeSearchWidget*);

	QMap<QString,AttributeSearchWidget*> pages_;
	QStandardItemModel* model_;
	QString query_;
};

#endif /* VIEWER_SRC_ATTRIBUTESEARCHPANEL_HPP_ */

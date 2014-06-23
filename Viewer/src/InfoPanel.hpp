//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPANEL_HPP_
#define INFOPANEL_HPP_

#include <QWidget>

#include "Viewer.hpp"
#include "ViewNodeInfo.hpp"

class QTabWidget;
class InfoPanel;
class InfoPanelItem;

class InfoPanelItemHandler
{
friend class InfoPanel;

public:
		InfoPanelItemHandler(QString id,QString name,InfoPanelItem* item) :
			id_(id), label_(name), item_(item) {}

		bool match(QStringList ids) const;
		QString id() const {return id_;}
		InfoPanelItem* item() const {return item_;}
		QWidget* widget();

protected:
		void addToTab(QTabWidget *);

private:
		QString id_;
		QString label_;
		InfoPanelItem* item_;
};


class InfoPanel : public QWidget
{
    Q_OBJECT

public:
   InfoPanel(QWidget* parent=0);
	virtual ~InfoPanel();

public slots:
	void slotReload(ViewNodeInfo_ptr node);
	void slotCurrentWidgetChanged(int);

protected:
	void adjust(QStringList);
	InfoPanelItemHandler* findHandler(QWidget* w);
	InfoPanelItemHandler* findHandler(QString id);
	InfoPanelItem* findItem(QWidget* w);
	InfoPanelItemHandler* createHandler(QString id);

	QTabWidget *tab_;
	QList<InfoPanelItemHandler*> items_;
	ViewNodeInfo_ptr currentNode_;
};

#endif

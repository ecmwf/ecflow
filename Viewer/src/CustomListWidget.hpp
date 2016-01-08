//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_CUSTOMLISTWIDGET_HPP_
#define VIEWER_SRC_CUSTOMLISTWIDGET_HPP_

#include <QListWidget>

class CustomListWidget : public QListWidget
{
Q_OBJECT

public:
	explicit CustomListWidget(QWidget* parent=0);

	void addItems(QStringList lst,bool checkState);
	void addItems(QStringList lst,bool checkState,QList<QColor>);
	QStringList selection() const;
	bool hasSelection() const;
	void setSelection(QStringList sel);

public Q_SLOTS:
	void clearSelection();

protected Q_SLOTS:
	void slotItemChanged(QListWidgetItem*);

Q_SIGNALS:
	void selectionChanged();


};


#endif /* VIEWER_SRC_CUSTOMLISTWIDGET_HPP_ */

//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CustomListWidget.hpp"

#include <QListWidgetItem>

CustomListWidget::CustomListWidget(QWidget* parent) : QListWidget(parent)
{
	setAlternatingRowColors(true);

	connect(this,SIGNAL(itemChanged(QListWidgetItem*)),
			this,SLOT(slotItemChanged(QListWidgetItem*)));
}

void CustomListWidget::addItems(QStringList lst,bool checkState)
{
	QListWidget::addItems(lst);
	for(int i=0; i < count(); i++)
	{
		item(i)->setCheckState((checkState)?Qt::Checked:Qt::Unchecked);
	}
}

void CustomListWidget::slotItemChanged(QListWidgetItem*)
{
	Q_EMIT selectionChanged();
}

bool CustomListWidget::hasSelection() const
{
	for(int i=0; i < count(); i++)
	{
		if(item(i)->checkState() == Qt::Checked)
			return true;
	}

	return false;
}


QStringList CustomListWidget::selection() const
{
	QStringList lst;
	for(int i=0; i < count(); i++)
	{
		if(item(i)->checkState() == Qt::Checked)
			lst << item(i)->text();
	}

	return lst;
}


void CustomListWidget::clearSelection()
{
	for(int i=0; i < count(); i++)
	{
		item(i)->setCheckState(Qt::Unchecked);
	}
	Q_EMIT selectionChanged();
}

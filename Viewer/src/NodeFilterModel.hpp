//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEFILTERMODEL_H
#define NODEFILTERMODEL_H

#include <QSortFilterProxyModel>

class NodeFilterModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
	NodeFilterModel(QObject *parent=0);
	~NodeFilterModel();

	bool filterAcceptsRow(int,const QModelIndex &) const;

	//From QSortFilterProxyModel
	void setSourceModel(QAbstractItemModel*);

public slots:
	void slotFilterChanged();

};

#endif

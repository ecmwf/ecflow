//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_NODEQUERYRESULTMODEL_HPP_
#define VIEWER_SRC_NODEQUERYRESULTMODEL_HPP_

#include <QAbstractItemModel>
#include <QColor>
#include <QSortFilterProxyModel>

#include <vector>

#include "NodeQueryResultData.hpp"
#include "VInfo.hpp"

class ModelColumn;

class NodeQueryResultModel : public QAbstractItemModel
{
Q_OBJECT

public:
   	explicit NodeQueryResultModel(QObject *parent=0);
   	~NodeQueryResultModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

    void clearData();
    bool hasData() const;

    VInfo_ptr nodeInfo(const QModelIndex&);
    QModelIndex infoToIndex(VInfo_ptr);

public Q_SLOTS:
	void appendRow(NodeQueryResultData);

protected:
	QList<NodeQueryResultData*> data_;
	ModelColumn* columns_;
};

#endif /* VIEWER_SRC_NODEQUERYRESULTMODEL_HPP_ */

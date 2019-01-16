//============================================================================
// Copyright 2009-2019 ECMWF.
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
//#include <QSortFilterProxyModel>

#include "VInfo.hpp"

class ModelColumn;
class NodeQueryResult;

class NodeQueryResultModel : public QAbstractItemModel
{
Q_OBJECT

public:
    enum CustomItemRole {SortRole = Qt::UserRole + 1};

   	explicit NodeQueryResultModel(QObject *parent=nullptr);
   	~NodeQueryResultModel() override;

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const override;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const override;

   	Qt::ItemFlags flags ( const QModelIndex & index) const override;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const override;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const override;
   	QModelIndex parent (const QModelIndex & ) const override;

   	NodeQueryResult *data() const {return data_;}
   	void clearData();
    bool hasData() const;
    //void addDataStart();
    //void addDataEnd();

    VInfo_ptr nodeInfo(const QModelIndex&);
    QModelIndex infoToIndex(VInfo_ptr);

    //To speed up identifying a column. The mapping here must match the definition of
    //"query_columns" in ecflowview_view_conf.json !!!
    enum ColumnType {ServerColumn=0,PathColumn=1,StatusColumn=2,TypeColumn=3,
                     StatusChangeColumn=4,AttributeColumn=5};

public Q_SLOTS:
    void slotBeginAppendRow();
    void slotEndAppendRow();
    void slotBeginAppendRows(int);
    void slotEndAppendRows(int);
    void slotBeginRemoveRow(int);
    void slotEndRemoveRow(int);
    void slotBeginRemoveRows(int,int);
    void slotEndRemoveRows(int,int);
    void slotBeginReset();
    void slotEndReset();
    void slotStateChanged(const VNode*,int,int);

protected:
	NodeQueryResult *data_;
	ModelColumn* columns_{nullptr};
};

#endif /* VIEWER_SRC_NODEQUERYRESULTMODEL_HPP_ */

//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_TRIGGERGRAPHMODEL_HPP_
#define VIEWER_SRC_TRIGGERGRAPHMODEL_HPP_

#include <QAbstractItemModel>
#include <QColor>

#include "VInfo.hpp"
#include "TriggerCollector.hpp"

class ModelColumn;
class NodeQueryResult;

class TriggerGraphModel : public QAbstractItemModel
{
Q_OBJECT

public:
    explicit TriggerGraphModel(QObject *parent=0);
    ~TriggerGraphModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

    TriggerListCollector* triggerCollector() const {return tc_;}
    void setTriggerCollector(TriggerListCollector *tc);

    //TriggerListCollector *triggers() const {return tc_;}
   	void clearData();
    bool hasData() const;
    //void addDataStart();
    //void addDataEnd();
    void beginUpdate();
    void endUpdate();

    VInfo_ptr nodeInfo(const QModelIndex&);
    QModelIndex infoToIndex(VInfo_ptr);

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
	TriggerListCollector* tc_;
	ModelColumn* columns_;
};

#endif /* VIEWER_SRC_TRIGGERGRAPHMODEL_HPP_ */

//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_TRIGGERTABLEMODEL_HPP_
#define VIEWER_SRC_TRIGGERTABLEMODEL_HPP_

#include <QAbstractItemModel>
#include <QColor>

#include "VInfo.hpp"
#include "TriggerCollector.hpp"

#include "Aspect.hpp"

class NodeQueryResult;

class TriggerTableModel : public QAbstractItemModel
{
public:
    enum Mode {TriggerMode,TriggeredMode};

    explicit TriggerTableModel(Mode mode,QObject *parent=0);
    ~TriggerTableModel();

    //The custom roles must have the same numerical value as in AbstractNodeModel.hpp because the
    //core delegate was written to only handle the custom roles it defines!
    enum CustomItemRole {FilterRole = Qt::UserRole+1, IconRole = Qt::UserRole+2,
                         NodeTypeRole = Qt::UserRole + 13,
                         NodeTypeForegroundRole = Qt::UserRole + 14};

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

    TriggerTableCollector* triggerCollector() const {return tc_;}
    void setTriggerCollector(TriggerTableCollector *tc);

   	void clearData();
    bool hasData() const;

    void beginUpdate();
    void endUpdate();

    TriggerTableItem* indexToItem(const QModelIndex&) const;
    VInfo_ptr nodeInfo(const QModelIndex&);
    QModelIndex itemToIndex(TriggerTableItem*);

    void nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>&);

protected:
    TriggerTableCollector* tc_;
    Mode mode_;
};

#endif /* VIEWER_SRC_TRIGGERTABLEMODEL_HPP_ */

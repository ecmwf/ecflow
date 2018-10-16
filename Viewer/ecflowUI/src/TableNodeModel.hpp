//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TABLENODEMODEL_H
#define TABLENODEMODEL_H

#include <QAbstractItemModel>

#include "AbstractNodeModel.hpp"
#include "VInfo.hpp"

class ModelColumn;
class Node;
class NodeFilterDef;
class ServerFilter;
class ServerHandler;
class VTableModelData;
class VTableServer;

class TableNodeModel : public AbstractNodeModel
{
Q_OBJECT

  friend class TableNodeSortModel;
  
public:
  TableNodeModel(ServerFilter* serverFilter,NodeFilterDef* filterDef,QObject *parent=nullptr);

  int columnCount (const QModelIndex& parent = QModelIndex() ) const;
  int rowCount (const QModelIndex& parent = QModelIndex() ) const;
  
  QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
  QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;
  
  QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
  QModelIndex parent (const QModelIndex & ) const;
  
  VInfo_ptr nodeInfo(const QModelIndex&);
  void selectionChanged(QModelIndexList lst);
  
  VModelData* data() const;
  void removeColumn(QString);
  ModelColumn* columns() const {return columns_;}

  //To speed up identifying a column. The mapping here must match the definition of
  //"table_columns" in ecflowview_view_conf.json !!!
  enum ColumnType {InvalidColumn=-1, PathColumn=0,StatusColumn=1,TypeColumn=2,TriggerColumn=3,
           LabelColumn=4, EventColumn=5, MeterColumn=6, StatusChangeColumn=7,
           ExtraColumn = 8 };
   
public Q_SLOTS:
  void slotServerAddBegin(int);
  void slotServerAddEnd();
  void slotServerRemoveBegin(VModelServer* server,int);
  void slotServerRemoveEnd(int);
  
  void slotDataChanged(VModelServer*) {}
  void slotNodeChanged(VTableServer*,const VNode*);
  void slotAttributesChanged(VModelServer*,const VNode*) {}
  void slotBeginAddRemoveAttributes(VModelServer*,const VNode*,int,int) {}
  void slotEndAddRemoveAttributes(VModelServer*,const VNode*,int,int) {}
  
  void slotBeginServerScan(VModelServer* server,int);
  void slotEndServerScan(VModelServer* server,int);
  void slotBeginServerClear(VModelServer* server,int);
  void slotEndServerClear(VModelServer* server,int);

  void slotAppendColumnBegin();
  void slotAppendColumnEnd();
  void slotAddColumnsBegin(int,int);
  void slotAddColumnsEnd(int,int);
  void slotChangeColumnBegin(int idx);
  void slotChangeColumnEnd(int idx);
  void slotRemoveColumnsBegin(int,int);
  void slotRemoveColumnsEnd(int,int);

Q_SIGNALS:
  void filterChangeBegun();
  void filterChangeEnded();

protected:
  bool isServer(const QModelIndex & index) const {return false;}
  ServerHandler* indexToRealServer(const QModelIndex & index) const {return NULL;}
  VModelServer* indexToServer(const QModelIndex & index) const {return NULL;}
  QModelIndex serverToIndex(ServerHandler*) const {return QModelIndex();}
  
  QModelIndex nodeToIndex(VTableServer* server,const VNode* node, int column) const;
  QModelIndex nodeToIndex(const VNode*,int column=0) const;
  VNode* indexToNode( const QModelIndex & index) const;
  
  QModelIndex attributeToIndex(const VAttribute* a, int column=0) const;
  
  QVariant nodeData(const QModelIndex& index,int role) const;
  
  VTableModelData* data_;
  ModelColumn* columns_;
  int diagPixId_;
};

#endif

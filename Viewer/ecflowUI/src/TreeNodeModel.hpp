//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>

#include "AbstractNodeModel.hpp"
#include "Node.hpp"
#include "VAttributeType.hpp"
#include "Viewer.hpp"
#include "VInfo.hpp"

class AttributeFilter;
class NodeFilterDef;
class ServerFilter;
class ServerHandler;
class VModelServer;
class VTreeModelData;
class VTreeNode;
class VTreeServer;

class TreeNodeModel : public AbstractNodeModel
{
Q_OBJECT

public:
   	TreeNodeModel(ServerFilter* serverFilter,NodeFilterDef* filterDef,
                  AttributeFilter *atts,IconFilter* icons,QObject *parent=nullptr);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const override;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const override;

   	Qt::ItemFlags flags ( const QModelIndex & index) const override;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const override;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const override;
   	QModelIndex parent (const QModelIndex & ) const override;

    QModelIndex serverToIndex(ServerHandler*) const override;

    QModelIndex nodeToIndex(const VTreeNode*,int column=0) const;
    QModelIndex nodeToIndex(const VNode*,int column=0) const override;
    VTreeServer* indexToServer(const QModelIndex & index) const;
    VTreeServer* nameToServer(const std::string&) const;
    VTreeNode* indexToNode( const QModelIndex & index) const;
    VTreeNode* indexToServerOrNode( const QModelIndex & index) const;

    QModelIndex attributeToIndex(const VAttribute* a, int column=0) const override;

    VInfo_ptr nodeInfo(const QModelIndex& index) override;
    void setForceShow(VInfo_ptr);
    void selectionChanged(QModelIndexList lst);

    void setEnableServerToolTip(bool st) {serverToolTip_=st;}
    void setEnableNodeToolTip(bool st) {nodeToolTip_=st;}
    void setEnableAttributeToolTip(bool st) {attributeToolTip_=st;}

   	VModelData* data() const override;

public Q_SLOTS:
	void slotServerAddBegin(int row) override;
	void slotServerAddEnd() override;
    void slotServerRemoveBegin(VModelServer*,int) override;
    void slotServerRemoveEnd(int) override;

    void slotNodeChanged(VTreeServer*,const VTreeNode*);
    void slotAttributesChanged(VTreeServer*,const VTreeNode*);
    void slotBeginAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int);
    void slotEndAddRemoveAttributes(VTreeServer*,const VTreeNode*,int,int);
    void slotBeginFilterUpdateRemove(VTreeServer*,const VTreeNode*,int);
    void slotEndFilterUpdateRemove(VTreeServer*,const VTreeNode*,int);
    void slotBeginFilterUpdateAdd(VTreeServer*,const VTreeNode*,int);
    void slotEndFilterUpdateAdd(VTreeServer*,const VTreeNode*,int);
    void slotBeginFilterUpdateRemoveTop(VTreeServer*,int);
    void slotEndFilterUpdateRemoveTop(VTreeServer*,int);
    void slotBeginFilterUpdateInsertTop(VTreeServer*,int);
    void slotEndFilterUpdateInsertTop(VTreeServer*,int);

	//void slotResetBranch(VModelServer*,const VNode*);
    void slotDataChanged(VModelServer*) override;
    void slotBeginServerScan(VModelServer* server,int) override;
	void slotEndServerScan(VModelServer* server,int) override;
	void slotBeginServerClear(VModelServer* server,int) override;
	void slotEndServerClear(VModelServer* server,int) override;

    int iconNum(VNode*) const;
    bool isNode(const QModelIndex & index) const;
    bool isAttribute(const QModelIndex & index) const;

Q_SIGNALS:
    void clearBegun(const VTreeNode*);
    void scanEnded(const VTreeNode*);
    void filterUpdateRemoveBegun(const VTreeNode*);
    void filterUpdateAddEnded(const VTreeNode*);
    void filterChangeBegun();
    void filterChangeEnded();
    void firstScanEnded(const VTreeServer*);

#if 0
protected:
    QModelIndex forceShowNode(const VNode* node) const;
    QModelIndex forceShowAttribute(const VAttribute* a) const;
#endif

private:
    bool isServer(const QModelIndex& index) const;
    bool isServerForValid(const QModelIndex& index) const;

    ServerHandler* indexToServerHandler(const QModelIndex & index) const;
    QModelIndex serverToIndex(VModelServer* server) const;

    QModelIndex nodeToIndex(VTreeServer*,const VTreeNode*,int column=0) const;
    VTreeNode* indexToAttrParentNode(const QModelIndex & index) const;
    VTreeNode* indexToAttrParentOrNode(const QModelIndex & index,bool &itIsANode) const;
	QVariant serverData(const QModelIndex& index,int role) const;
    QVariant nodeData(const QModelIndex& index,int role,VTreeNode*) const;
    QVariant attributesData(const QModelIndex& index,int role,VTreeNode*) const;

	//Attribute filter
	VTreeModelData* data_;
	AttributeFilter* atts_;
	IconFilter* icons_;

    bool serverToolTip_;
    bool nodeToolTip_;
    bool attributeToolTip_;
};


#endif

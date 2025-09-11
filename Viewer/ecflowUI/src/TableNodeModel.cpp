/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TableNodeModel.hpp"

#include <QMetaMethod>

#include "DiagData.hpp"
#include "IconProvider.hpp"
#include "ModelColumn.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VConfig.hpp"
#include "VFilter.hpp"
#include "VIcon.hpp"
#include "VModelData.hpp"
#include "VNState.hpp"
#include "VNode.hpp"

// #define _UI_TABLENODEMODEL_DEBUG

// static int hitCount=0;

static std::map<TableNodeModel::ColumnType, VAttributeType*> attrTypes;
static VAttributeType* columnToAttrType(TableNodeModel::ColumnType ct);

VAttributeType* columnToAttrType(TableNodeModel::ColumnType ct) {
    auto it = attrTypes.find(ct);
    return (it != attrTypes.end()) ? it->second : 0;
}

//=======================================================
//
// TableNodeModel
//
//=======================================================

TableNodeModel::TableNodeModel(ServerFilter* serverFilter, NodeFilterDef* filterDef, QObject* parent)
    : AbstractNodeModel(parent),
      data_(nullptr),
      columns_(nullptr) {
    columns_ = ModelColumn::def("table_columns");

    Q_ASSERT(columns_);

    // Check the mapping between the enum and column ids (only for the non-extra columns!)
    Q_ASSERT(columns_->id(PathColumn) == "path");
    Q_ASSERT(columns_->id(StatusColumn) == "status");
    Q_ASSERT(columns_->id(TypeColumn) == "type");
    Q_ASSERT(columns_->id(TriggerColumn) == "trigger");
    Q_ASSERT(columns_->id(LabelColumn) == "label");
    Q_ASSERT(columns_->id(EventColumn) == "event");
    Q_ASSERT(columns_->id(MeterColumn) == "meter");
    Q_ASSERT(columns_->id(StatusChangeColumn) == "statusChange");

    if (attrTypes.empty()) {
        QList<ColumnType> ctLst;
        ctLst << TriggerColumn << LabelColumn << EventColumn << MeterColumn;
        Q_FOREACH (ColumnType ct, ctLst) {
            VAttributeType* t = VAttributeType::find(columns_->id(ct).toStdString());
            Q_ASSERT(t);
            attrTypes[ct] = t;
        }
    }

    // Create the data handler for the model.
    data_ = new VTableModelData(filterDef, this);

    data_->reset(serverFilter);

    // We need to react to changes in the extra columns!
    connect(columns_, SIGNAL(appendItemBegin()), this, SLOT(slotAppendColumnBegin()));

    connect(columns_, SIGNAL(appendItemEnd()), this, SLOT(slotAppendColumnEnd()));

    connect(columns_, SIGNAL(addItemsBegin(int, int)), this, SLOT(slotAddColumnsBegin(int, int)));

    connect(columns_, SIGNAL(addItemsEnd(int, int)), this, SLOT(slotAddColumnsEnd(int, int)));

    connect(columns_, SIGNAL(changeItemBegin(int)), this, SLOT(slotChangeColumnBegin(int)));

    connect(columns_, SIGNAL(changeItemEnd(int)), this, SLOT(slotChangeColumnEnd(int)));

    connect(columns_, SIGNAL(removeItemsBegin(int, int)), this, SLOT(slotRemoveColumnsBegin(int, int)));

    connect(columns_, SIGNAL(removeItemsEnd(int, int)), this, SLOT(slotRemoveColumnsEnd(int, int)));

    // pixmap
    diagPixId_ = IconProvider::add(":/viewer/diag.svg", "diag.svg");
}

VModelData* TableNodeModel::getData() const {
    return data_;
}

int TableNodeModel::columnCount(const QModelIndex& /*parent */) const {
    return columns_->count();
}

int TableNodeModel::rowCount(const QModelIndex& parent) const {
#ifdef _UI_TABLENODEMODEL_DEBUG
    UiLog().dbg() << "rowCount=" << parent;
#endif

    // There are no servers
    if (!hasData()) {
        return 0;
    }
    // We use only column 0
    else if (parent.column() > 0) {
        return 0;
    }
    //"parent" is the root
    else if (!parent.isValid()) {
        // The total number of nodes displayed
        int cnt = 0;
        for (int i = 0; i < data_->count(); i++) {
            if (!data_->server(i)->inScan()) {
                cnt += data_->numOfNodes(i);
            }
        }
#ifdef _UI_TABLENODEMODEL_DEBUG
        // UiLog().dbg() << "table count " << cnt;
#endif
        return cnt;
    }

    return 0;
}

QVariant TableNodeModel::data(const QModelIndex& index, int role) const {
    // Data lookup can be costly so we immediately return a default value for all
    // the cases where the default should be used.
    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole && role != Qt::BackgroundRole &&
                             role != IconRole && role != SortRole)) {
        return {};
    }

    // We only display nodes!!
    return nodeData(index, role);
}

QVariant TableNodeModel::nodeData(const QModelIndex& index, int role) const {
    VNode* vnode = indexToNode(index);
    if (!vnode || !vnode->node()) {
        return {};
    }

    if (index.column() < 0) {
        return {};
    }

    ColumnType id = ExtraColumn;
    if (index.column() < ExtraColumn) {
        id = static_cast<ColumnType>(index.column());
    }

    if (role == Qt::DisplayRole) {
        // QString id=columns_->id(index.column());

        if (id == PathColumn) {
            return QString::fromStdString(vnode->absNodePath());
        }
        else if (id == StatusColumn) {
            return vnode->stateName();
        }
        else if (id == TypeColumn) {
            return QString::fromStdString(vnode->nodeType());
        }

        // Attributes
        else if (id == EventColumn || id == LabelColumn || id == MeterColumn || id == TriggerColumn) {
            if (VAttribute* a = vnode->attributeForType(0, columnToAttrType(id))) {
                return a->data(true);
            }
            else {
                return {};
            }
        }

        else if (id == StatusChangeColumn) {
            QString s;
            vnode->statusChangeTime(s);
            return s;
        }
        // Extra columns added by the user - they all represent ecflow variables!!!
        else if (id == ExtraColumn) {
            Q_ASSERT(columns_->isExtra(index.column()));
            QString n = columns_->id(index.column());
            if (!n.isEmpty()) {
                // Standard variable column
                if (columns_->isEditable(index.column())) {
                    return QString::fromStdString(vnode->findInheritedVariable(n.toStdString()));
                }
                // extra diagnostic column
                else {
                    DiagData* diag = DiagData::instance();
                    int diagCol    = index.column() - columns_->diagStartIndex();
                    if (diagCol >= 0) {
                        return QString::fromStdString(diag->dataAt(vnode, diagCol));
                    }
                }
            }
        }
    }
    else if (role == Qt::BackgroundRole) {
        return vnode->stateColour();
    }
    else if (role == IconRole) {
        if (id == PathColumn) {
            return VIcon::pixmapList(vnode, nullptr);
        }
        else {
            return {};
        }
    }
    else if (role == SortRole) {
        if (id == MeterColumn) {
            if (VAttribute* a = vnode->attributeForType(0, columnToAttrType(id))) {
                std::string val;
                if (a->value("meter_value", val)) {
                    return QString::fromStdString(val).toInt();
                }
            }
            return -9999;
        }
        else if (id == StatusChangeColumn) {
            return vnode->statusChangeTime();
        }
    }

    return {};
}

QVariant TableNodeModel::headerData(const int section, const Qt::Orientation orient, const int role) const {
    if (orient != Qt::Horizontal) {
        return QAbstractItemModel::headerData(section, orient, role);
    }

    if (section < 0 || section >= columns_->count()) { // this can happen during a server reset
        return {};
    }

    if (role == Qt::DisplayRole) {
        return columns_->label(section);
    }
    // the id of the column
    else if (role == Qt::UserRole) {
        return columns_->id(section);
    }
    else if (role == Qt::ToolTipRole) {
        if (section < ExtraColumn) {
            return columns_->tooltip(section);
        }
        else if (columns_->isEditable(section)) {
            return tr("Displays the value of the given ecFlow variable (can be changed or removed)");
        }
        else {
            return tr("Extra diagnostic");
        }
    }
    else if (role == VariableRole) {
        return (section >= ExtraColumn && columns_->isEditable(section)) ? true : false;
    }
    else if (role == Qt::DecorationRole) {
        if (section >= ExtraColumn && !columns_->isEditable(section)) {
            return IconProvider::pixmap(diagPixId_, 12);
        }
    }

    return {};
}

QModelIndex TableNodeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasData() || row < 0 || column < 0 || parent.isValid()) {
        return {};
    }

    if (VNode* node = data_->nodeAt(row)) {
        return createIndex(row, column, node);
    }

    return {};
}

QModelIndex TableNodeModel::parent(const QModelIndex& /*child*/) const {
    // Parent is always the root!!!
    return {};
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

VNode* TableNodeModel::indexToNode(const QModelIndex& index) const {
    if (index.isValid()) {
        return static_cast<VNode*>(index.internalPointer());
    }
    return nullptr;
}

QModelIndex TableNodeModel::nodeToIndex(const VNode* node, int column) const {
    if (!node) {
        return {};
    }

    int row = 0;
    if ((row = data_->position(node)) != -1) {
        return createIndex(row, column, const_cast<VNode*>(node));
    }
    return {};
}

// Find the index for the node when we know what the server is!
QModelIndex TableNodeModel::nodeToIndex(VTableServer* server, const VNode* node, int column) const {
    if (!node) {
        return {};
    }

    int row = 0;
    if ((row = data_->position(server, node)) != -1) {
        return createIndex(row, column, const_cast<VNode*>(node));
    }
    return {};
}

QModelIndex TableNodeModel::attributeToIndex(const VAttribute* a, int column) const {
    if (!a) {
        return {};
    }

    VNode* node = a->parent();
    if (!node) {
        return {};
    }

    int row = 0;
    if ((row = data_->position(node)) != -1) {
        return createIndex(row, column, const_cast<VNode*>(node));
    }
    return {};
}

void TableNodeModel::selectionChanged(QModelIndexList /*lst*/) {
#if 0
    Q_FOREACH(QModelIndex idx,lst)
    {
        VInfo_ptr info=nodeInfo(idx);

        for(int i=0; i < data_->count(); i++)
        {
           VTableServer *ts=data_->server(i)->tableServer();
           Q_ASSERT(ts);
           ts->clearForceShow(info->item());
        }
    }
#endif
}

VInfo_ptr TableNodeModel::nodeInfo(const QModelIndex& index) {
    VNode* n = indexToNode(index);
    if (n) {
        return VInfoNode::create(n);
    }

    VInfo_ptr info;
    return info;
}

// Server is about to be added
void TableNodeModel::slotServerAddBegin(int /*row*/) {
}

// Addition of the new server has finished
void TableNodeModel::slotServerAddEnd() {
}

// Server is about to be removed
void TableNodeModel::slotServerRemoveBegin(VModelServer* server, int num) {
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

    if (num > 0) {
        int start = -1;
        int count = -1;
        data_->position(server->tableServer(), start, count);

        Q_ASSERT(start >= 0);
        Q_ASSERT(count == num);

        beginRemoveRows(QModelIndex(), start, start + count - 1);
    }
}

// Removal of the server has finished
void TableNodeModel::slotServerRemoveEnd(int num) {
    assert(active_ == true);

    if (num > 0) {
        endRemoveRows();
    }
}

// The node changed (it status etc)
void TableNodeModel::slotNodeChanged(VTableServer* server, const VNode* node) {
    if (!node) {
        return;
    }

    QModelIndex index = nodeToIndex(server, node, 0);

    if (!index.isValid()) {
        return;
    }

    Q_EMIT dataChanged(index, index);
}

void TableNodeModel::slotBeginServerScan(VModelServer* server, int num) {
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

#ifdef _UI_TABLENODEMODEL_DEBUG
    UiLog().dbg() << "TableNodeModel::slotBeginServerScan --> " << server->realServer()->name() << " " << num;
#endif

    if (num > 0) {
        int count = num;
        int start = data_->position(server->tableServer());
        beginInsertRows(QModelIndex(), start, start + count - 1);
    }
}

void TableNodeModel::slotEndServerScan(VModelServer* /*server*/, int num) {
    assert(active_ == true);

#ifdef _UI_TABLENODEMODEL_DEBUG
    UiLog().dbg() << "TableNodeModel::slotEndServerScan --> " << server->realServer()->name() << " " << num;
    QTime t;
    t.start();
#endif

    if (num > 0) {
        // The sort model cannot perform sort properly in endInsertRows():
        // it seems that it is not able to see the newly added rows yet!!!
        // So we skip sorting then force to do it after calling endInsertRows().
        // This seems to work!!!
        Q_EMIT skipSortingBegin();
        endInsertRows();
        Q_EMIT skipSortingEnd();
    }

#ifdef _UI_TABLENODEMODEL_DEBUG
    UiLog().dbg() << "  elapsed: " << t.elapsed() << " ms";
    UiLog().dbg() << "<-- slotEndServerScan";
#endif
}

void TableNodeModel::slotBeginServerClear(VModelServer* server, int num) {
    Q_ASSERT(active_ == true);
    Q_ASSERT(server);

    if (num > 0) {
        int start = -1;
        int count = -1;
        data_->position(server->tableServer(), start, count);

        Q_ASSERT(start >= 0);
        Q_ASSERT(count == num);

        beginRemoveRows(QModelIndex(), start, start + count - 1);
    }
}

void TableNodeModel::slotEndServerClear(VModelServer* /*server*/, int num) {
    assert(active_ == true);

    if (num > 0) {
        endRemoveRows();
    }
}

//=======================================
// Column management
//=======================================

void TableNodeModel::removeColumn(QString name) {
    Q_ASSERT(columns_);
    columns_->removeExtraItem(name);
}

void TableNodeModel::slotAppendColumnBegin() {
    int col = columnCount();
    beginInsertColumns(QModelIndex(), col, col);
}

void TableNodeModel::slotAppendColumnEnd() {
    endInsertColumns();
}

void TableNodeModel::slotAddColumnsBegin(int idxStart, int idxEnd) {
    beginInsertColumns(QModelIndex(), idxStart, idxEnd);
}

void TableNodeModel::slotAddColumnsEnd(int /*idxStart*/, int /*idxEnd*/) {
    endInsertColumns();
}

void TableNodeModel::slotChangeColumnBegin(int /*idx*/) {
}

void TableNodeModel::slotChangeColumnEnd(int idx) {
    Q_EMIT dataChanged(index(0, idx), index(rowCount(), idx));
}

void TableNodeModel::slotRemoveColumnsBegin(int idxStart, int idxEnd) {
    beginRemoveColumns(QModelIndex(), idxStart, idxEnd);
}

void TableNodeModel::slotRemoveColumnsEnd(int /*idxStart*/, int /*idxEnd*/) {
    endRemoveColumns();
}

/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TriggerTableModel.hpp"

#include <QDebug>

#include "IconProvider.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VIcon.hpp"
#include "VNode.hpp"

TriggerTableModel::TriggerTableModel(Mode mode, QObject* parent)
    : QAbstractItemModel(parent),
      tc_(nullptr),
      mode_(mode) {
}

TriggerTableModel::~TriggerTableModel() = default;

void TriggerTableModel::clearData() {
    beginResetModel();
    tc_ = nullptr;
    endResetModel();
}

void TriggerTableModel::beginUpdate() {
    beginResetModel();
}

void TriggerTableModel::endUpdate() {
    endResetModel();
}

void TriggerTableModel::setTriggerCollector(TriggerTableCollector* tc) {
    // beginResetModel();
    tc_ = tc;
    // endResetModel();
}

bool TriggerTableModel::hasData() const {
    if (tc_) {
        return tc_->size() > 0;
    }
    else {
        return false;
    }
}

int TriggerTableModel::columnCount(const QModelIndex& /*parent */) const {
    return 1;
}

int TriggerTableModel::rowCount(const QModelIndex& parent) const {
    if (!hasData()) {
        return 0;
    }

    // Parent is the root:
    if (!parent.isValid()) {
        return tc_->size();
    }

    return 0;
}

Qt::ItemFlags TriggerTableModel::flags(const QModelIndex& /*index*/) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TriggerTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !hasData() ||
        (role < Qt::UserRole && role != Qt::DisplayRole && role != Qt::BackgroundRole &&
         role != Qt::TextAlignmentRole && role != Qt::ToolTipRole && role != Qt::ForegroundRole)) {
        return {};
    }

    int row = index.row();
    if (row < 0 || row >= static_cast<int>(tc_->size())) {
        return {};
    }

    // QString id=columns_->id(index.column());

    const std::vector<TriggerTableItem*>& items = tc_->items();
    VItem* t                                    = items[row]->item();
    Q_ASSERT(t);

    if (index.column() == 0) {
        if (VAttribute* a = t->isAttribute()) {
            if (role == Qt::DisplayRole) {
                QStringList d = a->data();
                if (VNode* pn = a->parent()) {
                    d.append(QString::fromStdString(pn->absNodePath()));
                }
                return d;
            }
            else if (role == Qt::ToolTipRole) {
                return a->toolTip();
            }
        }
        else if (VNode* vnode = t->isNode()) {
            if (role == Qt::DisplayRole) {
                return QString::fromStdString(vnode->absNodePath());
            }
            else if (role == Qt::BackgroundRole) {
                if (vnode->isSuspended()) {
                    QVariantList lst;
                    lst << vnode->stateColour() << vnode->realStateColour();
                    return lst;
                }
                else {
                    return vnode->stateColour();
                }
            }
            else if (role == Qt::ForegroundRole) {
                return vnode->stateFontColour();
            }

            else if (role == Qt::ToolTipRole) {
                QString txt = vnode->toolTip();
                // txt+=VIcon::toolTip(vnode,icons_);
                return txt;
            }
            else if (role == IconRole) {
                return VIcon::pixmapList(vnode, nullptr);
            }
            else if (role == NodeTypeRole) {
                if (vnode->isTask()) {
                    return 2;
                }
                else if (vnode->isSuite()) {
                    return 0;
                }
                else if (vnode->isFamily()) {
                    return 1;
                }
                else if (vnode->isAlias()) {
                    return 3;
                }
                return 0;
            }
            else if (role == NodeTypeForegroundRole) {
                return vnode->typeFontColour();
            }
            else if (role == NodePointerRole) {
                return QVariant::fromValue((void*)vnode);
            }

            else if (role == Qt::TextAlignmentRole) {
                return (mode_ == NodeMode) ? Qt::AlignCenter : Qt::AlignLeft;
            }
        }
    }

    // We express the table cell background colour through the UserRole. The
    // BackgroundRole is already used for the node rendering
    if (role == Qt::UserRole && mode_ != NodeMode) {
        const std::set<TriggerCollector::Mode>& modes = items[row]->modes();
        if (modes.find(TriggerCollector::Normal) != modes.end()) {
            return {};
        }
        else {
            return QColor(233, 242, 247);
        }
    }

    return {};
}

QVariant TriggerTableModel::headerData(const int section, const Qt::Orientation orient, const int role) const {
    if (orient != Qt::Horizontal) {
        return QAbstractItemModel::headerData(section, orient, role);
    }

    return {};
}

QModelIndex TriggerTableModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasData() || row < 0 || column < 0) {
        return {};
    }

    // When parent is the root this index refers to a node or server
    if (!parent.isValid()) {
        return createIndex(row, column);
    }

    return {};
}

QModelIndex TriggerTableModel::parent(const QModelIndex& /*child*/) const {
    return {};
}

VInfo_ptr TriggerTableModel::nodeInfo(const QModelIndex& index) {
    // For invalid index no info is created.
    if (!index.isValid()) {
        VInfo_ptr res;
        return res;
    }

    if (index.row() >= 0 && index.row() <= static_cast<int>(tc_->items().size())) {
        TriggerTableItem* d = tc_->items()[index.row()];
        return VInfo::createFromItem(d->item());
    }

    VInfo_ptr res;
    return res;
}

QModelIndex TriggerTableModel::itemToIndex(TriggerTableItem* item) {
    if (item) {
        for (std::size_t i = 0; i < tc_->items().size(); i++) {
            if (tc_->items()[i] == item) {
                return index(i, 0);
            }
        }
    }

    return {};
}

TriggerTableItem* TriggerTableModel::indexToItem(const QModelIndex& index) const {
    if (!hasData()) {
        return nullptr;
    }

    int row = index.row();
    if (row < 0 || row >= static_cast<int>(tc_->size())) {
        return nullptr;
    }

    const std::vector<TriggerTableItem*>& items = tc_->items();
    return items[row];
}

void TriggerTableModel::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>&) {
    if (!hasData()) {
        return;
    }

    int num = tc_->items().size();
    for (int i = 0; i < num; i++) {
        if (VItem* item = tc_->items()[i]->item()) {
            if (VNode* n = item->isNode()) {
                if (n == node) {
                    QModelIndex idx = index(i, 0);
                    Q_EMIT dataChanged(idx, idx);
                }
            }

            else if (VAttribute* a = item->isAttribute()) {
                if (a->parent() == node) {
                    QModelIndex idx = index(i, 0);
                    Q_EMIT dataChanged(idx, idx);
                }
            }
        }
    }
}

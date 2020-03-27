//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerGraphModel.hpp"

#include "IconProvider.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VIcon.hpp"
#include "VNode.hpp"

#include <QDebug>

TriggerGraphModel::TriggerGraphModel(Mode mode,QObject *parent) :
          QAbstractItemModel(parent),
          mode_(mode)
{
}

TriggerGraphModel::~TriggerGraphModel()
= default;


void TriggerGraphModel::clearData()
{
    beginResetModel();
    if (node_) {
        delete node_;
        node_=nullptr;
    }
    triggerTc_=nullptr;
    triggeredTc_ = nullptr;
    endResetModel();
}

void TriggerGraphModel::setNode(VInfo_ptr info)
{
    beginResetModel();
    if (node_) {
        delete node_;
    }
    node_= new TriggerTableItem(info->item());
    triggerTc_=nullptr;
    triggeredTc_ = nullptr;
    endResetModel();
}

void TriggerGraphModel::beginUpdate()
{
    beginResetModel();
}

void TriggerGraphModel::endUpdate()
{
    endResetModel();
}

//must be protected by beginUpdate()/endUpdate()
void TriggerGraphModel::setTriggerCollectors(TriggerTableCollector *triggerTc, TriggerTableCollector *triggeredTc)
{
    triggerTc_ = triggerTc;
    triggeredTc_ = triggeredTc;
    Q_ASSERT(triggerTc_);
    Q_ASSERT(triggeredTc_);
}

bool TriggerGraphModel::hasData() const
{
    return (node_ != nullptr);
//        return false;

//    if(triggerTc_) {
//        Q_ASSERT(triggeredTc_);
//        return (triggerTc_->size() > 0 || triggeredTc_->size() > 0);
//    }
//    return false;
}

bool TriggerGraphModel::hasTrigger() const
{
    if (triggerTc_) {
        Q_ASSERT(node_);
        Q_ASSERT(triggeredTc_);
        return true;
    }
    return false;
}

int TriggerGraphModel::columnCount( const QModelIndex& /*parent */) const
{
     return 1;
}

int TriggerGraphModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid() )
    {
        int num = 0;
        if (node_) {
            num +=1;
            if (hasTrigger()) {
                num += triggerTc_->size() + triggeredTc_->size();
            }
        }
        return num;
    }

    return 0;
}

Qt::ItemFlags TriggerGraphModel::flags ( const QModelIndex & index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TriggerGraphModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData() || (role < Qt::UserRole &&
        role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::TextAlignmentRole &&
        role != Qt::ToolTipRole && role != Qt::ForegroundRole))
    {
        return QVariant();
    }

    TriggerTableItem *item = indexToItem(index);
    if (!item) {
        return QVariant();
    }
    VItem *t = item->item();
    Q_ASSERT(t);

    if(index.column() == 0)
    {
        if (role == ServerRole) {
            return -1;
        }

        if(VAttribute* a=t->isAttribute())
        {
            if(role == Qt::DisplayRole)
            {
                QStringList d=a->data();
                if(VNode* pn=a->parent())
                    d.append(QString::fromStdString(pn->absNodePath()));
                return d;
            }
            else if(role ==  Qt::ToolTipRole)
            {
                return a->toolTip();
            }

        }
        else if(VNode* vnode=t->isNode())
        {
            if(role == Qt::DisplayRole)
            {
                return QString::fromStdString(vnode->absNodePath());
            }
            else if(role == Qt::BackgroundRole)
            {
                if(vnode->isSuspended())
                {
                    QVariantList lst;
                    lst << vnode->stateColour() << vnode->realStateColour();
                    return lst;
                }
                else
                    return vnode->stateColour() ;
            }
            else if(role == Qt::ForegroundRole)
                return vnode->stateFontColour();

            else if(role == Qt::ToolTipRole)
            {
                QString txt=vnode->toolTip();
                //txt+=VIcon::toolTip(vnode,icons_);
                return txt;
            }
            else if(role == IconRole)
            {
                return VIcon::pixmapList(vnode,nullptr);
            }
            else if(role  == NodeTypeRole)
            {
                 if(vnode->isTask()) return 2;
                 else if(vnode->isSuite()) return 0;
                 else if(vnode->isFamily()) return 1;
                 else if(vnode->isAlias()) return 3;
                 return 0;
            }
            else if(role  == NodeTypeForegroundRole)
            {
                return vnode->typeFontColour();
            }
            else if(role  == NodePointerRole)
            {
                 return qVariantFromValue((void *) vnode);
            }

            else if(role  == Qt::TextAlignmentRole)
            {
                return( mode_==NodeMode)?Qt::AlignCenter:Qt::AlignLeft;
            }

        }
    }

    //We express the table cell background colour through the UserRole. The
    //BackgroundRole is already used for the node rendering
//    if(role == Qt::UserRole && mode_ != NodeMode)
//    {
//        const std::set<TriggerCollector::Mode>&  modes=items[row]->modes();
//        if(modes.find(TriggerCollector::Normal) != modes.end())
//            return QVariant();
//        else
//            return QColor(233,242,247);
//    }

    return QVariant();
}

QVariant TriggerGraphModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal)
            return QAbstractItemModel::headerData( section, orient, role );

    return QVariant();
}

QModelIndex TriggerGraphModel::index( int row, int column, const QModelIndex & parent ) const
{
    if(!hasData() || row < 0 || column < 0)
    {
        return {};
    }

    //When parent is the root this index refers to a node or server
    if(!parent.isValid())
    {
        return createIndex(row,column);
    }

    return {};

}

QModelIndex TriggerGraphModel::parent(const QModelIndex &child) const
{
    return {};
}

VInfo_ptr TriggerGraphModel::nodeInfo(const QModelIndex& index)
{
    if(TriggerTableItem* t = indexToItem(index)) {
        return VInfo::createFromItem(t->item());
    }

    VInfo_ptr res;
    return res;
}

QModelIndex TriggerGraphModel::nodeToIndex(const VNode *node) const
{
    if (!hasData() || !node)
        return {};

    if (node_->item() == node)
        return index(0, 0);

    if (hasTrigger()) {
        for (size_t i=0; i < triggerTc_->items().size(); i++) {
            if (triggerTc_->items()[i]->item() == node)
                return index(1+i, 0);
        }
        for (size_t i=0; i < triggeredTc_->items().size(); i++) {
            if (triggeredTc_->items()[i]->item() == node)
                return index(1 + triggerTc_->size() + i, 0);
        }
    }

    return {};
}

QModelIndex TriggerGraphModel::itemToIndex(TriggerTableItem *item)
{
//    if(item)
//    {
//         for(std::size_t i=0; i < tc_->items().size(); i++)
//             if(tc_->items()[i] == item)
//                 return index(i,0);
//    }

    return {};
}

TriggerTableItem* TriggerGraphModel::indexToItem(const QModelIndex& index) const
{
    if(!hasData() || !index.isValid())
        return nullptr;

    int row=index.row();
    if (row == 0) {
        return node_;
    } else if (hasTrigger()){
        row -= 1;
        if(row < 0 || row >= static_cast<int>(triggerTc_->size() + triggeredTc_->size()))
            return nullptr;
        else if(row < static_cast<int>(triggerTc_->size())) {
            const std::vector<TriggerTableItem*>& items=triggerTc_->items();
            return items[row];
        } else {
            const std::vector<TriggerTableItem*>& items=triggeredTc_->items();
            return items[row - triggerTc_->size()];
        }
    }
    return nullptr;
}

void TriggerGraphModel::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>&)
{
    if(!hasData())
        return;

//    int num=tc_->items().size();
//    for(int i=0; i < num; i++)
//    {
//        if(VItem* item=tc_->items()[i]->item())
//        {
//            if(VNode* n=item->isNode())
//            {
//                if(n == node)
//                {
//                    QModelIndex idx=index(i,0);
//                    Q_EMIT dataChanged(idx,idx);
//                }
//            }

//            else if(VAttribute* a=item->isAttribute())
//            {
//                if(a->parent() == node)
//                {
//                    QModelIndex idx=index(i,0);
//                    Q_EMIT dataChanged(idx,idx);
//                }
//            }
//        }
//    }
}

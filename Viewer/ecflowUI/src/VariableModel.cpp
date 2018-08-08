//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableModel.hpp"

#include <QtGlobal>
#include <QColor>

#include "ServerHandler.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VariableModelData.hpp"

QColor VariableModel::varCol_=QColor(40,41,42);
//QColor VariableModel::genVarCol_=QColor(34,51,136);
QColor VariableModel::genVarCol_=QColor(0,115,48);
QColor VariableModel::shadowCol_=QColor(130,130,130);
QColor VariableModel::blockBgCol_=QColor(122,122,122);
QColor VariableModel::blockFgCol_=QColor(255,255,255);

#define _UI_VARIABLEMODEL_DEBUG

//=======================================================================
//
// VariabletModel
//
//=======================================================================

VariableModel::VariableModel(VariableModelDataHandler* data,QObject *parent) :
          QAbstractItemModel(parent),
          data_(data)
{
	connect(data_,SIGNAL(reloadBegin()),
			this,SLOT(slotReloadBegin()));

	connect(data_,SIGNAL(reloadEnd()),
					this,SLOT(slotReloadEnd()));

    connect(data_,SIGNAL(clearBegin(int,int)),
                this,SLOT(slotClearBegin(int,int)));

    connect(data_,SIGNAL(clearEnd(int,int)),
                this,SLOT(slotClearEnd(int,int)));

    connect(data_,SIGNAL(loadBegin(int,int)),
                this,SLOT(slotLoadBegin(int,int)));

    connect(data_,SIGNAL(loadEnd(int,int)),
                this,SLOT(slotLoadEnd(int,int)));

	connect(data_,SIGNAL(dataChanged(int)),
                this,SLOT(slotDataChanged(int)));

    connect(data_,SIGNAL(rerunFilter()),
               this,SIGNAL(rerunFilter()));
}

bool VariableModel::hasData() const
{
	return (data_->count()  > 0);
}

int VariableModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return 2;
}

int VariableModel::rowCount( const QModelIndex& parent) const
{

	//Parent is the root: the item must be a node or a server
	if(!parent.isValid())
	{
		return data_->count();
	}
	//The parent is a server or a node
	else if(!isVariable(parent))
	{
		int row=parent.row();
		return data_->varNum(row);
	}

	//parent is a variable
	return 0;
}

Qt::ItemFlags VariableModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags;

	defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return defaultFlags;
}

QVariant VariableModel::data( const QModelIndex& index, int role ) const
{
	if( !index.isValid())
    {
		return QVariant();
	}

	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole &&
       role != ReadOnlyRole && role != Qt::ToolTipRole && role != GenVarRole && role != Qt::FontRole &&
       role != ShadowRole )
	{
		return QVariant();
	}

	int row=index.row();
	int level=indexToLevel(index);

	//Server or node
	if(level == 1)
	{
		if(role == ReadOnlyRole)
			return QVariant();

		if(role == Qt:: BackgroundRole)
            return blockBgCol_;
        
		else if(role == Qt::ForegroundRole)
            return blockFgCol_;
        

        VariableModelData *d=data_->data(row);
		if(!d)
		{
			return QVariant();
		}

		if(index.column() == 0)
		{
			if(role == Qt::DisplayRole)
			{
                if(index.row() ==0)
                    return "defined in " + QString::fromStdString(d->type()) + " " + QString::fromStdString(d->name());
                else
                    return "inherited from " + QString::fromStdString(d->type()) + " " + QString::fromStdString(d->name());
			}
		}

		return QVariant();
	}

	//Variables
	else if (level == 2)
	{
        VariableModelData *d=data_->data(index.parent().row());
		if(!d)
		{
			return QVariant();
		}

		if(role == Qt::ForegroundRole)
		{
            if(d->isShadowed(row))
            {
                return shadowCol_;
            }

            //Generated variable
            if(d->isGenVar(row) && index.column() == 0)
				return genVarCol_;
			else
				return varCol_;
		}
		else if(role == Qt::DisplayRole)
        {    
		    if(index.column() == 0)
		    {
                QString s=QString::fromStdString(d->name(row));
                //if(d->isGenVar(row))
                //    s+=" (g)";
                return s;
            }
            else if(index.column() == 1)
            {
                return QString::fromStdString(d->value(row));
            }
        }
        else if(role == Qt::ToolTipRole)
        {
            QString s="User defined variable";
            if(d->isGenVar(row))
            {
                s="Generated variable";
            }
            if(d->isReadOnly(row))
                s+= " (read only)";

            if(d->isShadowed(row))
            {
                s+=".<br>Please note that this variable is <b>shadowed</b> i.e. \
                   overwritten in one of the descendants of this node shown in this panel!";
            }
            return s;
        }
		else if(role == ReadOnlyRole)
        {
			return (d->isReadOnly(row))?true:false;
        }

        else if(role == GenVarRole)
        {
            return (d->isGenVar(row))?true:false;
        }

        else if(role == ShadowRole)
        {
            return (d->isShadowed(row))?true:false;
        }

		return QVariant();
	}

	return QVariant();
}

bool VariableModel::variable(const QModelIndex& idx, QString& name,QString& value,bool& genVar) const
{
	int block=-1;
	int row=-1;

	identify(idx,block,row);

	if(row < 0)
		return false;

	if(block >=0 && block < data_->count())
	{
		name=QString::fromStdString(data_->data(block)->name(row));
		value=QString::fromStdString(data_->data(block)->value(row));
		genVar=data_->data(block)->isGenVar(row);
		return true;
	}

	return false;
}

QVariant VariableModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole )
      		  return QAbstractItemModel::headerData( section, orient, role );

   	switch ( section )
	{
   	case 0: return tr("Name");  
    case 1: return tr("Value");
   	default: return QVariant();
   	}

    return QVariant();
}

QModelIndex VariableModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return {};
	}

	//When parent is the root this index refers to a node or server
	if(!parent.isValid())
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		return createIndex(row,column,quintptr(0));
#else
		return createIndex(row,column,0);
#endif
	}

	//We are under one of the nodes
	else
	{
		return createIndex(row,column,(parent.row()+1)*1000);
	}

	return QModelIndex();

}

QModelIndex VariableModel::parent(const QModelIndex &child) const
{
	if(!child.isValid())
		return {};

	int level=indexToLevel(child);
	if(level == 1)
			return QModelIndex();
	else if(level == 2)
	{
		int id=child.internalId();
		int r=id/1000-1;
#ifdef ECFLOW_QT5
		return createIndex(r,child.column(),quintptr(0));
#else
		return createIndex(r,child.column(),0);
#endif
	}

	return QModelIndex();
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

int VariableModel::indexToLevel(const QModelIndex& index) const
{
	if(!index.isValid())
		return 0;

	int id=index.internalId();
	if(id >=0 && id < 1000)
	{
			return 1;
	}
	return 2;
}

VariableModelData* VariableModel::indexToData(const QModelIndex& index) const
{
    int block=-1;
    return indexToData(index,block);
}

VariableModelData* VariableModel::indexToData(const QModelIndex& index,int& block) const
{
    int row;

    identify(index,block,row);

    if(block != -1)
        return data_->data(block);

    return nullptr;
}

VInfo_ptr VariableModel::indexToInfo(const QModelIndex& index) const
{
    if(VariableModelData* d=indexToData(index))
    {
        //It is a block
        QModelIndex p=index.parent();

        //It is a block
        //if(!index.parent().isValid()) <-- this did not work
        if(!p.isValid())
            return d->info();
        //it is a variable within a block
        else
            return d->info(index.row());
    }
    return VInfo_ptr();
}

QModelIndex VariableModel::infoToIndex(VInfo_ptr info) const
{
    if(!info)
        return {};

    int block=-1;
    int row=-1;
    data_->findVariable(info,block,row);

    if(block != -1)
    {
        QModelIndex blockIndex=index(block,0);
        if(row != -1)
        {
            return index(row,0,blockIndex);
        }
        return blockIndex;
    }

    return QModelIndex();
}

//----------------------------------------------
//
// Server to index mapping and lookup
//
//----------------------------------------------

bool VariableModel::isVariable(const QModelIndex & index) const
{
	return (indexToLevel(index) == 2);
}

void VariableModel::identify(const QModelIndex& index,int& block,int& row) const
{
    block=-1;
    row=-1;
    
    if(!index.isValid())
    {
       return;
    }
    
    int level=indexToLevel(index);
    
    if(level == 1)
    {
        block=index.row();
        row=-1;
    }
    else if(level == 2)
    {
        block=parent(index).row();
        row=index.row();       
    }        
}    

void VariableModel::slotReloadBegin()
{
	//Reset the whole model
	beginResetModel();
}

void VariableModel::slotReloadEnd()
{
	endResetModel();
}

void VariableModel::slotClearBegin(int block,int num)
{
#ifdef _UI_VARIABLEMODEL_DEBUG
    UI_FUNCTION_LOG
#endif
    QModelIndex parent=index(block,0);
    if(!parent.isValid())
        return;

    int rc=rowCount(parent);
    UI_ASSERT(num >= 0," num=" << num);
    UI_ASSERT(num == rc," num=" << num <<
              " rowCount=" << rowCount(parent));

    if(num > 0)
    {
        beginRemoveRows(parent,0,num-1);
    }
}

void VariableModel::slotClearEnd(int block,int num)
{
#ifdef _UI_VARIABLEMODEL_DEBUG
    UI_FUNCTION_LOG
#endif

    QModelIndex parent=index(block,0);
    if(!parent.isValid())
        return;

    UI_ASSERT(num >= 0,"num=" << num);
    if(num > 0)
    {
        endRemoveRows();
    }
}

void VariableModel::slotLoadBegin(int block,int num)
{
#ifdef _UI_VARIABLEMODEL_DEBUG
    UI_FUNCTION_LOG
#endif
    QModelIndex parent=index(block,0);
    if(!parent.isValid())
        return;

    int rc=rowCount(parent);
    UI_ASSERT(num >= 0,"num=" << num);
    UI_ASSERT(rc==0,"rowCount=" << rowCount(parent));

    if(num > 0)
    {
        beginInsertRows(parent,0,num-1);
    }
}

void VariableModel::slotLoadEnd(int block,int num)
{
#ifdef _UI_VARIABLEMODEL_DEBUG
    UI_FUNCTION_LOG
#endif
    QModelIndex parent=index(block,0);
    if(!parent.isValid())
        return;

    UI_ASSERT(num >= 0,"num=" << num);
    if(num > 0)
    {
        endInsertRows();
    }
}

//It must be called after any data change
void VariableModel::slotDataChanged(int block)
{
#ifdef _UI_VARIABLEMODEL_DEBUG
    UI_FUNCTION_LOG
#endif
    QModelIndex blockIndex0=index(block,0);
	QModelIndex blockIndex1=index(block,1);

#ifdef _UI_VARIABLEMODEL_DEBUG
    UiLog().dbg() << " emit dataChanged:" << " " << blockIndex0 << " " << blockIndex1;
#endif

    //This will sort and filter the block
    Q_EMIT dataChanged(blockIndex0,blockIndex1);
}

//=======================================================================
//
// VariableSortModel
//
//=======================================================================

VariableSortModel::VariableSortModel(VariableModel *varModel,QObject* parent) :
	QSortFilterProxyModel(parent),
	varModel_(varModel),
    showShadowed_(true),
	matchMode_(FilterMode),
	ignoreDuplicateNames_(false)
{
	QSortFilterProxyModel::setSourceModel(varModel_);
	setDynamicSortFilter(true);

    connect(varModel_,SIGNAL(filterChanged()),
            this,SLOT(slotFilterChanged()));

    connect(varModel_,SIGNAL(rerunFilter()),
            this,SLOT(slotRerunFilter()));
}

void VariableSortModel::setMatchMode(MatchMode mode)
{
	if(matchMode_ == mode)
		return;

	matchMode_=mode;

	matchLst_.clear();
	matchText_.clear();

	//reload the filter model
	invalidate();
}

void VariableSortModel::setMatchText(QString txt)
{
	matchText_=txt;

	if(matchMode_ == FilterMode)
	{
		//reload the filter model
		invalidate();
	}
}

void VariableSortModel::print(const QModelIndex idx)
{
    if(rowCount(idx) > 0)
        UiLog().dbg() << "--> " << idx << " " << mapToSource(idx) << " " << data(idx);
    else
        UiLog().dbg() << idx << " " << mapToSource(idx) << " " << data(idx);

    if(rowCount(idx) > 0)  UiLog().dbg() << "children: ";
    for(int i=0; i < rowCount(idx); i++)
    {
        print(index(i,0,idx));
    }
}

void VariableSortModel::slotFilterChanged()
{
    if(matchMode_ == FilterMode)
    {
        invalidate();
    }
}

void VariableSortModel::slotRerunFilter()
{
#ifdef _UI_VARIABLEMODEL_DEBUG
   UiLog().dbg() << "VariableSortModel::slotRerunFilter-->";
#endif     
   invalidate();
}

void VariableSortModel::slotShowShadowed(bool b)
{
   if(showShadowed_ != b)
    {
        showShadowed_=b;
        invalidate();
    }
}

bool VariableSortModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
	//Node or server. Here we want the nodes and server to stay unsorted. That is the order should stay as
	//it is defined in the data handler: the selected node stays on top and its ancestors and the server
	//follow each other downwards. This order is reflected in the row index of these items in
	//the varModel: the selected node's row is 0, its parent's row is 1, etc.
	if(!varModel_->isVariable(sourceLeft))
	{
		if(sortOrder() == Qt::AscendingOrder)
			return (sourceLeft.row() < sourceRight.row());
		else
			return (sourceLeft.row() > sourceRight.row());
	}
	//For variables we simply sort according to the string
	else
	{
    //UiLog().dbg() << varModel_->data(sourceLeft,Qt::DisplayRole).toString() << " " << varModel_->data(sourceRight,Qt::DisplayRole).toString();
        return varModel_->data(sourceLeft,Qt::DisplayRole).toString() < varModel_->data(sourceRight,Qt::DisplayRole).toString();
	}
	return true;
}

bool VariableSortModel::filterAcceptsRow(int sourceRow,const QModelIndex& sourceParent) const
{
    if(!sourceParent.isValid())
        return true;

    QModelIndex idx=varModel_->index(sourceRow,0,sourceParent);

    if(!showShadowed_)
    {
        if(varModel_->data(idx,VariableModel::ShadowRole).toBool())
            return false;
    }

    if(matchMode_ != FilterMode || matchText_.simplified().isEmpty())
		return true;

	QModelIndex idx2=varModel_->index(sourceRow,1,sourceParent);

	QString s=varModel_->data(idx,Qt::DisplayRole).toString();
	QString s2=varModel_->data(idx2,Qt::DisplayRole).toString();

	if(s.contains(matchText_,Qt::CaseInsensitive) || s2.contains(matchText_,Qt::CaseInsensitive))
	{
		return true;
	}

	return false;
}

QVariant VariableSortModel::data(const QModelIndex& idx,int role) const
{
    if(role != Qt::UserRole)
    {
        return QSortFilterProxyModel::data(idx,role);
    }    
    
    //We highlight the matching items (the entire row).
    if(matchMode_ == SearchMode && matchLst_.count() >0)
    {
        int col2=(idx.column()==0)?1:0;
        QModelIndex idx2=index(idx.row(),col2,idx.parent());

        if(matchLst_.contains(idx) || matchLst_.contains(idx2))
            return QColor(169,210,176);
    }
    
    return QSortFilterProxyModel::data(idx,role);
}    


QModelIndexList VariableSortModel::match(const QModelIndex& start,int role,const QVariant& value,int hits,Qt::MatchFlags flags) const
{
	if(matchMode_ != SearchMode)
		return QModelIndexList();

	QModelIndex root;
	matchText_=value.toString();

	matchLst_.clear();

	if(matchText_.simplified().isEmpty())
		return matchLst_;

	for(int i=0; i < rowCount(); i++)
	{
		QModelIndex idx=index(i,0);
		for(int row=0; row < rowCount(idx);row++)
		{
			//Name column
			QModelIndex colIdx=index(row,0,idx);
			QString s=data(colIdx,Qt::DisplayRole).toString();

			if(s.contains(matchText_,Qt::CaseInsensitive))
			{
				matchLst_ << colIdx;
				continue;
			}

			//Value columns
			QModelIndex colIdx1=index(row,1,idx);
			s=data(colIdx1,Qt::DisplayRole).toString();
			if(s.contains(matchText_,Qt::CaseInsensitive))
			{
				matchLst_ << colIdx;
			}
		}
	}

	return matchLst_;
}

#if 0
void VariableSortModel::test()
{
    UI_FUNCTION_LOG
    QModelIndex idx;
    test(idx);
    testSource(idx);
}

void VariableSortModel::test(const QModelIndex& p)
{
    int num=rowCount(p);
    for(int i=0; i < num; i++)
    {
        QModelIndex idx=index(i,0,p);
        QModelIndex sIdx=mapToSource(idx);
        if(!sIdx.isValid())
        {
            UiLog().dbg() << " idx=" << idx;
            Q_ASSERT(sIdx.isValid());
        }
        //UiLog().dbg() << idx.data().toString() << " " << sIdx.data().toString();

        if(idx.data().toString() != sIdx.data().toString())
        {
            UI_ASSERT(0,"filter=" << idx.data().toString() <<
                      " source=" << sIdx.data().toString());
        }

        test(idx);
    }
}

void VariableSortModel::testSource(const QModelIndex& p)
{
    int num=varModel_->rowCount(p);
    for(int i=0; i < num; i++)
    {
        QModelIndex sIdx=varModel_->index(i,0,p);
        QModelIndex idx=mapFromSource(sIdx);
        if(!idx.isValid())
        {
            UiLog().dbg() << " idx=" << idx;
            Q_ASSERT(sIdx.isValid());
        }
        if(idx.data().toString() != sIdx.data().toString())
        {
            UI_ASSERT(0,"filter=" << idx.data().toString() <<
                      " source=" << sIdx.data().toString());
        }
        testSource(idx);
    }
}
#endif

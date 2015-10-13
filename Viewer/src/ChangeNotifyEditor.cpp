//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotifyEditor.hpp"

#include "PropertyLine.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"

#include <QDebug>
#include <QVBoxLayout>

#include <assert.h>

ChangeNotifyEditor::ChangeNotifyEditor(QWidget* parent) : QWidget(parent)
{
	setupUi(this);

	model_=new ChangeNotifyEditorModel(this);

	tree_->setModel(model_);

	connect(tree_,SIGNAL(activated(QModelIndex)),
			this,SLOT(slotRowSelected(QModelIndex)));

	connect(tree_,SIGNAL(clicked(QModelIndex)),
			this,SLOT(slotRowSelected(QModelIndex)));

	assert(stacked_->count()==0);

	QFont f;
	QFontMetrics fm(f);
	tree_->setFixedHeight((fm.height()+4)*5.5);
}

void ChangeNotifyEditor::addRow(QString label,QList<PropertyLine*> lineLst,QWidget *stackContents)
{
	PropertyLine* enabledLine=0;
	PropertyLine* popupLine=0;
	PropertyLine* soundLine=0;

	QList<VProperty*> propLst;
	Q_FOREACH(PropertyLine* pl,lineLst)
	{
		if(pl)
		{
			propLst << pl->property();

			if(pl->property()->name() == "enabled")
			{
				enabledLine=pl;
			}
			if(pl->property()->name() == "popup")
			{
				popupLine=pl;
			}
			if(pl->property()->name() == "sound")
			{
				soundLine=pl;
			}

		}
	}

	if(enabledLine)
	{
		connect(model_,SIGNAL(enabledChanged(VProperty*,QVariant)),
				enabledLine,SLOT(slotReset(VProperty*,QVariant)));

		connect(enabledLine,SIGNAL(changed(QVariant)),
				model_,SLOT(slotEnabledChanged(QVariant)));

		connect(enabledLine,SIGNAL(masterChanged(bool)),
				model_,SLOT(slotEnabledMasterChanged(bool)));

		if(popupLine)
		{
			connect(enabledLine,SIGNAL(changed(QVariant)),
					popupLine,SLOT(slotEnabled(QVariant)));
			//init
			popupLine->slotEnabled(enabledLine->property()->value());
		}
		if(soundLine)
		{
			connect(enabledLine,SIGNAL(changed(QVariant)),
					soundLine,SLOT(slotEnabled(QVariant)));

			//init
			soundLine->slotEnabled(enabledLine->property()->value());
		}
	}

	model_->add(label,propLst);

	QWidget* w=new QWidget(this);
	QVBoxLayout* vb=new QVBoxLayout();
	vb->setContentsMargins(0,5,0,5);
	w->setLayout(vb);
	vb->addWidget(stackContents);
	vb->addStretch(1);

	stacked_->addWidget(w);

	tree_->setCurrentIndex(model_->index(0,0));

	for(int i=0; i < model_->columnCount()-1; i++)
	{
		tree_->resizeColumnToContents(i);
	}
}

void ChangeNotifyEditor::slotRowSelected(const QModelIndex& idx)
{
	if(idx.row() == stacked_->currentIndex())
		return;

	if(idx.row() >= 0 && idx.row() < stacked_->count())
	{
		stacked_->setCurrentIndex(idx.row());
	}
}

ChangeNotifyEditorModel::ChangeNotifyEditorModel(QObject *parent) :
     QAbstractItemModel(parent)
{

}

ChangeNotifyEditorModel::~ChangeNotifyEditorModel()
{
}

void ChangeNotifyEditorModel::add(QString label,QList<VProperty*> propLst)
{
	beginResetModel();

	ChangeNotifyEditorModelData d;

	d.label_=label;

	Q_FOREACH(VProperty* p,propLst)
	{
		if(p)
		{
			if(p->name() == "enabled")
			{
				d.enabled_=p;
				d.enabledVal_=p->value().toBool();
				d.enabledMaster_=(p->master() && p->useMaster());

				//Get the description
				if(p->parent())
				{
					if(VProperty* np=VConfig::instance()->find("notification."+ p->parent()->strName()))
						d.desc_=np->param("description");
				}
			}
		}
	}

	data_ << d;

	endResetModel();
}

int ChangeNotifyEditorModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int ChangeNotifyEditorModel::rowCount( const QModelIndex& parent) const
{
	//Parent is the root:
	if(!parent.isValid())
	{
		return data_.count();
	}

	return 0;
}

QVariant ChangeNotifyEditorModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid())
    {
		return QVariant();
	}

	int row=index.row();
	if(row < 0 || row >= data_.count())
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 1:
			return data_.at(row).label_;
		case 2:
			return data_.at(row).desc_;
		default:
			return QVariant();
		}
	}

	else if(role == Qt::CheckStateRole)
	{
		switch(index.column())
		{
			case 0:
				return (data_.at(row).enabledVal_)?QVariant(Qt::Checked):QVariant(Qt::Unchecked);
			default:
				return QVariant();
		}
	}

	return QVariant();
}

bool ChangeNotifyEditorModel::setData(const QModelIndex& index,const QVariant& value, int role)
{
	if(index.column() == 0)
	{
		int row=index.row();
		if(row <0 || row >= data_.count())
			return false;

		if(role == Qt::CheckStateRole)
		{
			bool checked=(value.toInt() == Qt::Checked)?true:false;
			data_[row].enabledVal_=checked;
			Q_EMIT dataChanged(index,index);
			Q_EMIT enabledChanged(data_[row].enabled_,checked);
			return true;
		}
	}

	return false;
}


QVariant ChangeNotifyEditorModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole)
      		  return QAbstractItemModel::headerData( section, orient, role );

   	if(role == Qt::DisplayRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Enabled ");
   		case 1: return tr("Title");
   		case 2: return tr("Description");
   		default: return QVariant();
   		}
   	}

    return QVariant();
}

QModelIndex ChangeNotifyEditorModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(row < 0 || column < 0)
	{
		return QModelIndex();
	}

	//When parent is the root this index refers to a node or server
	if(!parent.isValid())
	{
		return createIndex(row,column);
	}

	return QModelIndex();

}

QModelIndex ChangeNotifyEditorModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

Qt::ItemFlags ChangeNotifyEditorModel::flags( const QModelIndex & index) const
{
	int row=index.row();
	if(row >=0 && row <= data_.count() &&
	   index.column() ==0 && data_.at(row).enabledMaster_)
	{
		return Qt::ItemIsUserCheckable| Qt::ItemIsSelectable;
	}

	Qt::ItemFlags defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if(index.isValid() && index.column() ==0)
		defaultFlags=defaultFlags | Qt::ItemIsUserCheckable;

	return defaultFlags;
}

int ChangeNotifyEditorModel::lineToRow(PropertyLine* line) const
{
	for(int i=0; i < data_.count(); i++)
	{
		if(data_.at(i).enabled_ == line->property())
		{
			return i;
		}
	}
	return -1;
}


void ChangeNotifyEditorModel::slotEnabledChanged(QVariant v)
{
	PropertyLine *line=static_cast<PropertyLine*>(sender());
	assert(line);

	int row=lineToRow(line);
	if(row != -1)
	{
		//We want to avoid circular dependencies (e.g. this function is triggered from
		//setData. So we have to check if the value is different from the stored one!
		if(data_.at(row).enabledVal_ != v.toBool())
		{
			data_[row].enabledVal_=v.toBool();
				QModelIndex idx=index(row,0);
				Q_EMIT dataChanged(idx,idx);
		}
	}
}

void ChangeNotifyEditorModel::slotEnabledMasterChanged(bool b)
{
	PropertyLine *line=static_cast<PropertyLine*>(sender());
	assert(line);

	int row=lineToRow(line);
	if(row != -1)
	{
		QModelIndex idx=index(row,0);
		data_[row].enabledMaster_=b;
		if(b)
		{
			data_[row].enabledMaster_=b;
		}
		Q_EMIT dataChanged(idx,idx);
	}
}


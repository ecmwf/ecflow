//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerDialog.hpp"

#include "ServerFilter.hpp"
#include "ServerList.hpp"

#include <QDialogButtonBox>
#include <QIntValidator>
#include <QMessageBox>
#include <QSettings>
#include <QSortFilterProxyModel>

//======================================
//
// ServerSelectionItem
//
//======================================

ServerSelectionItem::~ServerSelectionItem()
{
	delete server_;
}

//======================================
//
// ServerSelectionData
//
//======================================

ServerSelectionData::ServerSelectionData(ServerFilter* filter)
{
	for(int i=0; i < ServerList::Instance()->count(); i++)
	{
		items_.push_back(new ServerSelectionItem(ServerList::Instance()->item(i)->clone(),false));
	}

	if(filter)
	{
		for(unsigned int i=0; i < filter->servers().size(); i++)
		{
			bool found=false;
			for(std::vector<ServerSelectionItem*>::iterator it=items_.begin(); it !=items_.end(); it++)
			{
						if((*it)->server()->match(filter->servers().at(i)))
						{
							(*it)->selected(true);
							found = true;
							break;
						}
				}
				if(!found)
				{
					items_.push_back(new ServerSelectionItem(filter->servers().at(i)->clone(),true));
				}
			}
		}
}

ServerSelectionData::~ServerSelectionData()
{
	for(std::vector<ServerSelectionItem*>::iterator it=items_.begin(); it !=items_.end(); it++)
			delete *it;
}

void ServerSelectionData::add(ServerItem *server,bool selection)
{
	items_.push_back(new ServerSelectionItem(server->clone(),selection));
}

void ServerSelectionData::remove(ServerItem* server)
{
	for(std::vector<ServerSelectionItem*>::iterator it=items_.begin(); it !=items_.end(); it++)
	{
		if((*it)->server() == server)
		{
			delete *it;
			items_.erase(it);
			return;
		}
	}
}

ServerItem* ServerSelectionData::server(int index) const
{
	if(index >=0 && index < items_.size())
		return items_.at(index)->server();
	return 0;
}

bool ServerSelectionData::isSelected(int index) const
{
	if(index >=0 && index < items_.size())
		return items_.at(index)->selected();
	return 0;
}

void ServerSelectionData::selected(int index,bool st)
{
	if(index >=0 && index < items_.size())
		items_.at(index)->selected(st);
}

void ServerSelectionData::selectedServers(std::vector<ServerItem*>& vec)
{
	for(std::vector<ServerSelectionItem*>::iterator it=items_.begin(); it !=items_.end(); it++)
	{
		if((*it)->selected())
			vec.push_back((*it)->server());
	}
}

void ServerSelectionData::allServers(std::vector<ServerItem*>& vec)
{
	for(std::vector<ServerSelectionItem*>::iterator it=items_.begin(); it !=items_.end(); it++)
	{
		vec.push_back((*it)->server());
	}
}

//======================================
//
// ServerSelectionModel
//
//======================================

ServerSelectionModel::ServerSelectionModel(ServerSelectionData* data,QObject *parent) :
  QAbstractItemModel(parent)
{
	data_=data;
}

ServerSelectionModel::~ServerSelectionModel()
{
}

void ServerSelectionModel::dataIsAboutToChange()
{
	beginResetModel();
}

void ServerSelectionModel::dataChangeFinished()
{
	endResetModel();
}

int ServerSelectionModel::columnCount(const QModelIndex& parent) const
{
	return 3;
}

int ServerSelectionModel::rowCount(const QModelIndex& parent) const
{
	if(!parent.isValid())
		return data_->count();

	return 0;
}

QVariant ServerSelectionModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid() ||
	   (role != Qt::DisplayRole && role != Qt::CheckStateRole))
	{
		return QVariant();

	}

	ServerItem* item=data_->server(index.row());
	if(!item)
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0: return QString::fromStdString(item->name());
		case 1: return QString::fromStdString(item->host());
		case 2: return QString::fromStdString(item->port());
		default: return QVariant();
		}
	}
	else if(role == Qt::CheckStateRole)
	{
		if(index.column() == 0)
		{
			return (data_->isSelected(index.row()))?QVariant(Qt::Checked):QVariant(Qt::Unchecked);
		}
	}

	return QVariant();
}

bool ServerSelectionModel::setData(const QModelIndex& index, const QVariant&  value, int role)
{
	if(!index.isValid())
	{
		return false;
	}

	if(index.column() == 0)
	{
		if(role == Qt::CheckStateRole)
		{
			bool checked=(value.toInt() == Qt::Checked)?true:false;

			data_->selected(index.row(),checked);

			emit dataChanged(index,index);

			return true;
		}
	}

	return false;
}

QVariant ServerSelectionModel::headerData(int section,Qt::Orientation ori,int role) const
{
	if(ori != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section > 2)
	{
		return QVariant();
	}

    switch(section)
	{
		case 0: return tr("Name");
		case 1: return tr("Host");
		case 2: return tr("Port");
		default: return QVariant();
	}

    return QVariant();
}

QModelIndex ServerSelectionModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
	return createIndex(row,column,0);
}

QModelIndex ServerSelectionModel::parent(const QModelIndex &) const
{
	return QModelIndex();
}

Qt::ItemFlags ServerSelectionModel::flags( const QModelIndex &index) const
{
	//return QAbstractItemModel::flags(index);

	Qt::ItemFlags defaultFlags;

	defaultFlags=Qt::ItemIsEnabled |
		  	Qt::ItemIsSelectable;

	if(index.column() ==0)
	{
		defaultFlags=defaultFlags | Qt::ItemIsUserCheckable; //| Qt::ItemIsEditable;
	}

	return defaultFlags;
}

ServerItem* ServerSelectionModel::indexToServer(const QModelIndex& index)
{
	return data_->server(index.row());
}

//======================================
//
// ServerItemDialog
//
//======================================

ServerItemDialog::ServerItemDialog(ServerItem *item,QWidget *parent) :
   QDialog(parent),
   item_(item)
{
	setupUi(this);

	nameEdit->setText(QString::fromStdString(item_->name()));
	hostEdit->setText(QString::fromStdString(item_->host()));
	portEdit->setText(QString::fromStdString(item_->port()));

	//Validators
	//nameEdit->setValidator(new QRegExpValidator(""));
	//hostEdit->setValidator(new QRegExpValidator(""));
	portEdit->setValidator(new QIntValidator(1025,65535,this));
}

void ServerItemDialog::accept()
{
	//Check name
	QString name=nameEdit->text();
	if(name.simplified().isEmpty())
	{
		error(tr("<b>Name</b> cannot be empty!"));
		return;
	}
	else if(name.contains(","))
	{
		error(tr("<b>Name</b> cannot contain comma character!"));
		return;
	}

	QString host=hostEdit->text();
	if(host.simplified().isEmpty())
	{
		error(tr("<b>Host</b> cannot be empty!"));
		return;
	}
	else if(host.contains(","))
	{
		error(tr("<b>Host</b> cannot contain comma character!"));
		return;
	}

	QString port=portEdit->text();
	if(port.simplified().isEmpty())
	{
		error(tr("<b>Port</b> cannot be empty!"));
		return;
	}
	else if(port.contains(","))
	{
		error(tr("<b>Port</b> cannot contain comma character!"));
		return;
	}

	item_->name(name.toStdString());
	item_->host(host.toStdString());
	item_->port(port.toStdString());

	QDialog::accept();
}

void ServerItemDialog::reject()
{
  	QDialog::reject();
}

void ServerItemDialog::error(QString msg)
{
	QString errTxt=tr("Failed to save settings!<br>");
	QMessageBox::critical(this,tr("Server item"),errTxt + msg);
}

//======================================
//
// ServerDialog
//
//======================================

ServerDialog::ServerDialog(ServerFilter *filter,QWidget *parent) :
   QDialog(parent),
   filter_(filter),
   data_(0)
{
    setupUi(this);

    data_=new ServerSelectionData(filter);

    sortModel_=new QSortFilterProxyModel(this);
    model_=new ServerSelectionModel(data_,this);
    sortModel_->setSourceModel(model_);
    sortModel_->setDynamicSortFilter(true);

    serverView->setModel(sortModel_);

    //Buttonbox
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    readSettings();

}

ServerDialog::~ServerDialog()
{
	if(data_)
		delete data_;

	writeSettings();

}

void ServerDialog::editItem(const QModelIndex& index)
{
	if(ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index)))
	{
		ServerItemDialog d(item,this);
		d.exec();
	}
}

void ServerDialog::addItem()
{
	ServerItem* item=new ServerItem("");
	ServerItemDialog d(item,this);
	if(d.exec() == QDialog::Accepted)
	{
		model_->dataIsAboutToChange();
		//Data does not take ownership of the item, just clone it!
		data_->add(item,false);
		model_->dataChangeFinished();
	}

	delete item;
}

void ServerDialog::removeItem(const QModelIndex& index)
{
	ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index));

	if(!item)
		return;

	QString str=tr("Are you sure that you want to delete server: <b>");
	str+=QString::fromStdString(item->name()) ;
	str+="</b>?";

	QMessageBox msgBox;
	msgBox.setWindowTitle(tr("Delete server"));
	msgBox.setText(str);
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	int ret=msgBox.exec();

	switch (ret)
	{
   	case QMessageBox::Yes:
   		model_->dataIsAboutToChange();
   		//It will delete item as well!
   		data_->remove(item);
   		model_->dataChangeFinished();
      	break;
   	case QMessageBox::Cancel:
       	// Cancel was clicked
       	break;
   	default:
       	// should never be reached
      	 break;
	}
}


void ServerDialog::on_editTb_clicked()
{
	QModelIndex index=serverView->currentIndex();
	editItem(index);
}

void ServerDialog::on_serverView_doubleClicked(const QModelIndex& index)
{
	editItem(index);
}

void ServerDialog::on_addTb_clicked()
{
	addItem();
}

void ServerDialog::on_deleteTb_clicked()
{
	QModelIndex index=serverView->currentIndex();
	removeItem(index);
}

void ServerDialog::on_rescanTb_clicked()
{

}

void ServerDialog::saveSelection()
{
	std::vector<ServerItem*> vecAll;
	data_->allServers(vecAll);
	ServerList::Instance()->update(vecAll);

	if(filter_)
	{
		std::vector<ServerItem*> vecSel;
		data_->selectedServers(vecSel);
		filter_->update(vecSel);
	}
}

void ServerDialog::accept()
{
	saveSelection();
  	QDialog::accept();
}

void ServerDialog::reject()
{
  	QDialog::reject();
}

void ServerDialog::writeSettings()
{
	QSettings settings("ECMWF","ECF-ServerDialog");

	//We have to clear it not to remember all the previous windows
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void ServerDialog::readSettings()
{
	QSettings settings("ECMWF","ECF-ServerDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(350,500));
	}

	settings.endGroup();
}


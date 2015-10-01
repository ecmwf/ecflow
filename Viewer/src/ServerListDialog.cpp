//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerListDialog.hpp"

#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>

#include "ServerFilter.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"

//======================================
//
// ServerDialogChecked
//
//======================================

bool ServerDialogChecker::checkName(QString name,QString oriName)
{
	if(name.simplified().isEmpty())
	{
		error(QObject::tr("<b>Name</b> cannot be empty!"));
		return false;
	}
	else if(name.contains(","))
	{
		error(QObject::tr("<b>Name</b> cannot contain comma character!"));
		return false;
	}

    if(oriName != name && ServerList::instance()->find(name.toStdString()) )
	{           
        error(QObject::tr("The specified server already exists! Please select a different name!"));
        return false;
	}

	return true;
}

bool ServerDialogChecker::checkHost(QString host)
{
	if(host.simplified().isEmpty())
	{
		error(QObject::tr("<b>Host</b> cannot be empty!"));
		return false;
	}
	else if(host.contains(","))
	{
		error(QObject::tr("<b>Host</b> cannot contain comma character!"));
		return false;
	}

	return true;
}

bool ServerDialogChecker::checkPort(QString port)
{
	if(port.simplified().isEmpty())
	{
		error(QObject::tr("<b>Port</b> cannot be empty!"));
		return false;
	}
	else if(port.contains(","))
	{
		error(QObject::tr("<b>Port</b> cannot contain comma character!"));
		return false;
	}
	return true;
}


void ServerDialogChecker::error(QString msg)
{
	QMessageBox::critical(0,QObject::tr("Server item"),errorText_ + "<br>"+ msg);
}

//======================================
//
// ServerAddDialog
//
//======================================

ServerAddDialog::ServerAddDialog(QWidget *parent) :
   QDialog(parent),
   ServerDialogChecker(tr("Cannot create new server!"))
{
	setupUi(this);

	//nameEdit->setText();
	//hostEdit->setText();
	//portEdit->setText();
	addToCurrentCb->setChecked(true);

	//Validators
	//nameEdit->setValidator(new QRegExpValidator(""));
	//hostEdit->setValidator(new QRegExpValidator(""));
	portEdit->setValidator(new QIntValidator(1025,65535,this));
}

void ServerAddDialog::accept()
{
	QString name=nameEdit->text();
	QString host=hostEdit->text();
	QString port=portEdit->text();

	if(!checkName(name) || !checkHost(host) || !checkPort(port))
		return;

	QDialog::accept();
}

QString ServerAddDialog::name() const
{
	return nameEdit->text();
}

QString ServerAddDialog::host() const
{
	return hostEdit->text();
}

QString ServerAddDialog::port() const
{
	return portEdit->text();
}

bool ServerAddDialog::addToView() const
{
	return addToCurrentCb->isChecked();
}

//======================================
//
// ServerEditDialog
//
//======================================

ServerEditDialog::ServerEditDialog(QString name, QString host, QString port,QWidget *parent) :
   QDialog(parent),
   ServerDialogChecker(tr("Cannot modify server!")),
   oriName_(name)
{
	setupUi(this);

	nameEdit->setText(name);
	hostEdit->setText(host);
	portEdit->setText(port);

	//Validators
	//nameEdit->setValidator(new QRegExpValidator(""));
	//hostEdit->setValidator(new QRegExpValidator(""));
	portEdit->setValidator(new QIntValidator(1025,65535,this));
}

void ServerEditDialog::accept()
{
	QString name=nameEdit->text();
	QString host=hostEdit->text();
	QString port=portEdit->text();

    if(!checkName(name,oriName_) || !checkHost(host) || !checkPort(port))
		return;

	QDialog::accept();
}


QString ServerEditDialog::name() const
{
	return nameEdit->text();
}

QString ServerEditDialog::host() const
{
	return hostEdit->text();
}

QString ServerEditDialog::port() const
{
	return portEdit->text();
}

//======================================
//
// ServerListDialog
//
//======================================

ServerListDialog::ServerListDialog(Mode mode,ServerFilter *filter,QWidget *parent) :
	QDialog(parent),
	mode_(mode),
	filter_(filter)
{
	setupUi(this);

	sortModel_=new QSortFilterProxyModel(this);
	model_=new ServerListModel(this);
	sortModel_->setSourceModel(model_);
	sortModel_->setDynamicSortFilter(true);

	serverView->setRootIsDecorated(false);
	serverView->setAllColumnsShowFocus(true);
	serverView->setUniformRowHeights(true);
	serverView->setSortingEnabled(true);
	serverView->setSelectionMode(QAbstractItemView::ExtendedSelection);

	serverView->setModel(sortModel_);

	//Add context menu actions to the view
	QAction* sep1=new QAction(this);
    sep1->setSeparator(true);
    QAction* sep2=new QAction(this);
    sep2->setSeparator(true);

	serverView->addAction(actionAdd);
	serverView->addAction(sep1);
	serverView->addAction(actionDuplicate);
	serverView->addAction(actionEdit);
	serverView->addAction(sep2);
	serverView->addAction(actionDelete);

	//Add actions for the toolbuttons
	addTb->setDefaultAction(actionAdd);
	deleteTb->setDefaultAction(actionDelete);
	editTb->setDefaultAction(actionEdit);
	rescanTb->setDefaultAction(actionRescan);


	buttonBox->button(QDialogButtonBox::Ok)->hide();
	buttonBox->button(QDialogButtonBox::Cancel)->hide();

	//Load settings
	readSettings();
}

ServerListDialog::~ServerListDialog()
{
	writeSettings();
}

void ServerListDialog::accept()
{
	//Overwrite ServerList with the actual data
  	QDialog::accept();
}

void ServerListDialog::editItem(const QModelIndex& index)
{
	if(ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index)))
	{
		ServerEditDialog d(QString::fromStdString(item->name()),
						   QString::fromStdString(item->host()),
						   QString::fromStdString(item->port()),this);

		//The dialog checks the name, host and port!
		if(d.exec()== QDialog::Accepted)
		{
			ServerList::instance()->reset(item,d.name().toStdString(),d.host().toStdString(),d.port().toStdString());
		}
	}
}

void ServerListDialog::duplicateItem(const QModelIndex& index)
{
	if(ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index)))
	{
		std::string dname=ServerList::instance()->uniqueName(item->name());

		ServerEditDialog d(QString::fromStdString(dname),
						   QString::fromStdString(item->host()),
						   QString::fromStdString(item->port()),this);

		//The dialog checks the name, host and port!
		if(d.exec() == QDialog::Accepted)
		{
			model_->dataIsAboutToChange();
			ServerList::instance()->add(d.name().toStdString(),d.host().toStdString(),d.port().toStdString());
			model_->dataChangeFinished();
		}
	}
}

void ServerListDialog::addItem()
{
	ServerAddDialog d(this);

	//The dialog checks the name, host and port!
	if(d.exec() == QDialog::Accepted)
	{
		model_->dataIsAboutToChange();
		ServerItem* item=ServerList::instance()->add(d.name().toStdString(),d.host().toStdString(),d.port().toStdString());
		model_->dataChangeFinished();
		if(d.addToView() && filter_)
		{
			filter_->addServer(item);
		}
	}
}

void ServerListDialog::removeItem(const QModelIndex& index)
{
	ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index));

	if(!item)
		return;

	QString str=tr("Are you sure that you want to delete server: <b>");
	str+=QString::fromStdString(item->name()) ;
	str+="</b>?";
	str+=tr("<br>It will be removed from all the existing views!");

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
   		ServerList::instance()->remove(item);
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

void ServerListDialog::on_serverView_doubleClicked(const QModelIndex& index)
{
	editItem(index);
}

void ServerListDialog::on_actionEdit_triggered()
{
	QModelIndex index=serverView->currentIndex();
	editItem(index);
}

void ServerListDialog::on_actionAdd_triggered()
{
	addItem();
}

void ServerListDialog::on_actionDelete_triggered()
{
	QModelIndex index=serverView->currentIndex();
	removeItem(index);
}

void ServerListDialog::on_actionDuplicate_triggered()
{
	QModelIndex index=serverView->currentIndex();
	duplicateItem(index);
}


void ServerListDialog::on_actionRescan_triggered()
{

}

void ServerListDialog::writeSettings()
{
	QSettings settings("ECMWF","ECF-ServerListDialog");

	//We have to clear it not to remember all the previous windows
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void ServerListDialog::readSettings()
{
	QSettings settings("ECMWF","ECF-ServerListDialog");

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

//======================================
//
// ServerListModel
//
//======================================

ServerListModel::ServerListModel(QObject *parent) :
  QAbstractItemModel(parent)
{
}

ServerListModel::~ServerListModel()
{
}

void ServerListModel::dataIsAboutToChange()
{
	beginResetModel();
}

void ServerListModel::dataChangeFinished()
{
	endResetModel();
}

int ServerListModel::columnCount(const QModelIndex& parent) const
{
	return 4;
}

int ServerListModel::rowCount(const QModelIndex& parent) const
{
	if(!parent.isValid())
		return static_cast<int>(ServerList::instance()->count());

	return 0;
}

QVariant ServerListModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid() ||
	  (role != Qt::DisplayRole && role != Qt::ForegroundRole))
	{
		return QVariant();
	}

	ServerItem* item=ServerList::instance()->itemAt(index.row());


	if(!item)
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0: return QString::fromStdString(item->name());
		case 1: return QString::fromStdString(item->host());
		case 2: return QString::fromStdString(item->port());
		case 3:
		{
			QString txt;
			/*if(data_->isFiltered(item))
				txt=tr("current view");
			else if(item->isUsed())
				txt=tr("other (1)");*/
			return txt;
		}
		default: return QVariant();
		}
	}
	else if (role == Qt::ForegroundRole)
	{
		/*if(data_->isFiltered(item))
				return Qt::blue;
		else if(item->isUsed())
			return Qt::green;
		else*/
			return QVariant();
	}

	return QVariant();
}

bool ServerListModel::setData(const QModelIndex& index, const QVariant&  value, int role)
{
	if(!index.isValid())
	{
		return false;
	}

	return false;
}

QVariant ServerListModel::headerData(int section,Qt::Orientation ori,int role) const
{
	if(ori != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section > 3)
	{
		return QVariant();
	}

    switch(section)
	{
		case 0: return tr("Name");
		case 1: return tr("Host");
		case 2: return tr("Port");
		case 3: return tr("Usage");
		default: return QVariant();
	}

    return QVariant();
}

QModelIndex ServerListModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
	return createIndex(row,column,static_cast<void*>(0));
}

QModelIndex ServerListModel::parent(const QModelIndex &) const
{
	return QModelIndex();
}

ServerItem* ServerListModel::indexToServer(const QModelIndex& index)
{
	return ServerList::instance()->itemAt(index.row());
}

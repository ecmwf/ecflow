//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerListDialog.hpp"

#include <QtGlobal>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>

#include "IconProvider.hpp"
#include "ServerFilter.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"
#include "ServerListSyncWidget.hpp"
#include "SessionHandler.hpp"
#include "VConfig.hpp"
#include "WidgetNameProvider.hpp"

static bool firstShowSysSyncLogW=true;

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
	QMessageBox::critical(nullptr,QObject::tr("Server item"),errorText_ + "<br>"+ msg);
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

ServerEditDialog::ServerEditDialog(QString name, QString host, QString port,bool favourite,QWidget *parent) :
   QDialog(parent),
   ServerDialogChecker(tr("Cannot modify server!")),
   oriName_(name)
{
	setupUi(this);

	nameEdit->setText(name);
	hostEdit->setText(host);
	portEdit->setText(port);
	favCh->setChecked(favourite);

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

bool ServerEditDialog::isFavourite() const
{
	return favCh->isChecked();
}

//======================================
//
// ServerListDialog
//
//======================================

ServerListDialog::ServerListDialog(Mode mode,ServerFilter *filter,QWidget *parent) :
	QDialog(parent),
    filter_(filter),
    mode_(mode)
{
	setupUi(this);

	QString wt=windowTitle();
	wt+="  -  " + QString::fromStdString(VConfig::instance()->appLongName());
	setWindowTitle(wt);

	sortModel_=new ServerListFilterModel(this);
	model_=new ServerListModel(filter_,this);
	sortModel_->setSourceModel(model_);
	sortModel_->setDynamicSortFilter(true);

	serverView->setRootIsDecorated(false);
	serverView->setAllColumnsShowFocus(true);
	serverView->setUniformRowHeights(true);
	serverView->setAlternatingRowColors(true);
	serverView->setSortingEnabled(true);
	serverView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	serverView->sortByColumn(ServerListModel::NameColumn,Qt::AscendingOrder);

	serverView->setModel(sortModel_);

	//Add context menu actions to the view
	auto* sep1=new QAction(this);
    sep1->setSeparator(true);
    auto* sep2=new QAction(this);
    sep2->setSeparator(true);
    auto* sep3=new QAction(this);
    sep3->setSeparator(true);

	serverView->addAction(actionAdd);
	serverView->addAction(sep1);
	serverView->addAction(actionDuplicate);
	serverView->addAction(actionEdit);
	serverView->addAction(sep2);
	serverView->addAction(actionDelete);
	serverView->addAction(sep3);
	serverView->addAction(actionFavourite);

	//Add actions for the toolbuttons
	addTb->setDefaultAction(actionAdd);
	deleteTb->setDefaultAction(actionDelete);
	editTb->setDefaultAction(actionEdit);
	duplicateTb->setDefaultAction(actionDuplicate);
	//rescanTb->setDefaultAction(actionRescan);

	checkActionState();

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
	filterLe_->setPlaceholderText(tr("Filter"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    filterLe_->setClearButtonEnabled(true);
#endif

	connect(filterLe_,SIGNAL(textEdited(QString)),
			 this,SLOT(slotFilter(QString)));

	connect(filterFavTb_,SIGNAL(clicked(bool)),
	    	this,SLOT(slotFilterFavourite(bool)));

	//Load settings
	readSettings();

	//The selection changes in the view
	connect(serverView->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));

	connect(serverView,SIGNAL(clicked(QModelIndex)),
				this,SLOT(slotItemClicked(QModelIndex)));


	for(int i=0; i < model_->columnCount()-1; i++)
		serverView->resizeColumnToContents(i);

#if 0
    QFont labelF;
    labelF.setBold(true);
    labelF.setPointSize(labelF.pointSize()-1);

    systemListLabel->setFont(labelF);
    systemListLabel->setText("<font color=\'#565656\'>" + systemListLabel->text() + "</font>");
#endif
    systemListLabel->hide();

    //At the moment we do not want users to sync manually. It is done automatically
    //on each startup.
    sysSyncTb->hide();

    //The synclog is hidden
    sysSyncLogTb->setChecked(false);
    sysSyncLogW_->hide();
    if(!ServerList::instance()->hasSystemFile())
    {
        sysSyncLogTb->setEnabled(false);
    }

    //Assign name to each object
    WidgetNameProvider::nameChildren(this);
}

ServerListDialog::~ServerListDialog()
{
    writeSettings();
}

void ServerListDialog::closeEvent(QCloseEvent* event)
{
	event->accept();
	writeSettings();
	ServerList::instance()->save();
}

void ServerListDialog::accept()
{
	//Overwrite ServerList with the actual data
	writeSettings();
	QDialog::accept();
	ServerList::instance()->save();
}

void ServerListDialog::reject()
{
	//Overwrite ServerList with the actual data
	writeSettings();
	QDialog::reject();
	ServerList::instance()->save();
}

void ServerListDialog::editItem(const QModelIndex& index)
{
	if(ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index)))
	{
        if(item->isSystem())
            return;

        ServerEditDialog d(QString::fromStdString(item->name()),
						   QString::fromStdString(item->host()),
						   QString::fromStdString(item->port()),
						   item->isFavourite(),this);

		//The dialog checks the name, host and port!
		if(d.exec()== QDialog::Accepted)
		{
			ServerList::instance()->reset(item,d.name().toStdString(),d.host().toStdString(),d.port().toStdString());

			if(item->isFavourite()  != d.isFavourite())
			{
				ServerList::instance()->setFavourite(item,d.isFavourite());
				QModelIndex idx=sortModel_->index(index.row(),ServerListModel::FavouriteColumn);
				serverView->update(idx);
			}
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
                           QString::fromStdString(item->port()),item->isFavourite(),this);

		//The dialog checks the name, host and port!
		if(d.exec() == QDialog::Accepted)
		{
			model_->dataIsAboutToChange();
			ServerList::instance()->add(d.name().toStdString(),d.host().toStdString(),d.port().toStdString(),false);
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
        ServerItem* item=nullptr;
        try {
            item=ServerList::instance()->add(d.name().toStdString(),d.host().toStdString(),d.port().toStdString(),false);
        }
        catch(std::exception& e)
        {

        }

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


void ServerListDialog::setFavouriteItem(const QModelIndex& index,bool b)
{
	ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index));

	if(!item)
		return;

	ServerList::instance()->setFavourite(item,b);

	QModelIndex idx=sortModel_->index(index.row(),ServerListModel::FavouriteColumn);
	serverView->update(idx);
}


void ServerListDialog::on_serverView_doubleClicked(const QModelIndex& index)
{
	int col=index.column();
	if(col == ServerListModel::NameColumn || col == ServerListModel::HostColumn ||
	   col == ServerListModel::PortColumn)
	{
		editItem(index);
	}
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

void ServerListDialog::on_actionFavourite_triggered(bool checked)
{
	QModelIndex index=serverView->currentIndex();
	setFavouriteItem(index,checked);
}

void ServerListDialog::on_actionRescan_triggered()
{

}

void ServerListDialog::on_sysSyncTb_clicked(bool)
{
#if 0
    ServerList::instance()->syncSystemFile();
    on_sysSyncLogTb_toggled(true);
#endif
}

void ServerListDialog::on_sysSyncLogTb_toggled(bool b)
{
    sysSyncLogW_->setVisible(b);
    if(b && firstShowSysSyncLogW)
    {
        firstShowSysSyncLogW=false;

        //Set the initial splitter sizes
        QList<int> sList=splitter_->sizes();       
        Q_ASSERT(sList.count()==2);
        int h=sList[0]+sList[1];
        if(h==0)
        {
           sList[0]=75;
           sList[1]=25;
        }
        else
        {
            sList[1]=h/2;
            if(ServerList::instance()->hasSyncChange())
            {
                if(h > 500)
                    sList[1]=250;
            }
            else
            {
                if(h > 100)
                    sList[1]=50;
            }
            sList[0]=h-sList[1];
        }

        splitter_->setSizes(sList);
    }
}

void ServerListDialog::showSysSyncLog()
{
    sysSyncLogTb->setChecked(true);
}

void ServerListDialog::slotItemSelected(const QModelIndex& current,const QModelIndex& prev)
{
	checkActionState();
}

void ServerListDialog::slotItemClicked(const QModelIndex& current)
{
	if(ServerItem* item=model_->indexToServer(sortModel_->mapToSource(current)))
	{
		if(current.column() == ServerListModel::FavouriteColumn)
		{
			setFavouriteItem(current,!item->isFavourite());
		}
	}

	checkActionState();
}

void ServerListDialog::checkActionState()
{
	QModelIndex index=serverView->currentIndex();

	if(!index.isValid())
	{
		actionEdit->setEnabled(false);
		actionDuplicate->setEnabled(false);
		actionDelete->setEnabled(false);
		actionFavourite->setEnabled(false);
	}
	else
	{
		ServerItem* item=model_->indexToServer(sortModel_->mapToSource(index));

		assert(item);

        actionEdit->setEnabled(!item->isSystem());
		actionDuplicate->setEnabled(true);
        actionDelete->setEnabled(!item->isSystem());
		actionFavourite->setEnabled(true);
		actionFavourite->setChecked(item->isFavourite());
	}
}

void ServerListDialog::slotFilter(QString txt)
{
	sortModel_->setFilterStr(txt);
}

void ServerListDialog::slotFilterFavourite(bool b)
{
	sortModel_->setFilterFavourite(b);
}

void ServerListDialog::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ServerListDialog")),
                       QSettings::NativeFormat);

	//We have to clear it not to remember all the previous windows
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.setValue("filterFav",filterFavTb_->isChecked());
	settings.endGroup();
}

void ServerListDialog::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ServerListDialog")),
                       QSettings::NativeFormat);

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(350,500));
	}

	if(settings.contains("filterFav"))
	{
		//This does not emit the clicked signal
		filterFavTb_->setChecked(settings.value("filterFav").toBool());
		//so we need to do it explicitly
		sortModel_->setFilterFavourite(filterFavTb_->isChecked());
	}

	settings.endGroup();
}

//======================================
//
// ServerListModel
//
//======================================

ServerListModel::ServerListModel(ServerFilter* filter,QObject *parent) :
  QAbstractItemModel(parent),
  filter_(filter)
{
	int id=IconProvider::add(":/viewer/favourite.svg","favourite");
	favPix_=IconProvider::pixmap(id,12);

	id=IconProvider::add(":/viewer/favourite_empty.svg","favourite_empty");
	favEmptyPix_=IconProvider::pixmap(id,12);

    id=IconProvider::add(":/viewer/system.svg","system");
    sysPix_=IconProvider::pixmap(id,12);

	loadFont_.setBold(true);
}

ServerListModel::~ServerListModel()
= default;

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
    return 6;
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
	  (role != Qt::DisplayRole && role != Qt::ForegroundRole && role != Qt::DecorationRole &&
              role != Qt::CheckStateRole  && role != Qt::UserRole && role != Qt::FontRole &&
              role != IconStatusRole && role != Qt::ToolTipRole))
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
		case NameColumn: return QString::fromStdString(item->name());
		case HostColumn: return QString::fromStdString(item->host());
        case PortColumn: return QString::fromStdString(item->port());
        case UseColumn:
		{
            int n=item->useCnt();
            if(n > 0)
                return "loaded (" + QString::number(n) + ")";

			return QVariant();
		}
		default: return QVariant();
		}
	}
	else if (role == Qt::ForegroundRole)
	{
        //return (item->isSystem())?QColor(70,71,72):QVariant();
        return (item->isSystem())?QColor(67,78,109):QVariant();
	}
	else if (role == Qt::DecorationRole)
	{
       if(index.column() == SystemColumn)
            return (item->isSystem())?sysPix_:QPixmap();

        else if(index.column() == FavouriteColumn)
			return (item->isFavourite())?favPix_:favEmptyPix_;

		return QVariant();
	}
	else if (role == Qt::UserRole)
	{
		if(index.column() == FavouriteColumn)
			return item->isFavourite();

		return QVariant();
	}
	else if (role == Qt::CheckStateRole)
	{
		if(index.column() == LoadColumn && filter_)
			return (filter_->isFiltered(item))?QVariant(Qt::Checked):QVariant(Qt::Unchecked);

		return QVariant();
	}
	else if (role == Qt::FontRole)
	{
		if(index.column() != LoadColumn && index.column() != FavouriteColumn &&
		  filter_ && filter_->isFiltered(item))
			  return loadFont_;

		return QVariant();
	}
    else if (role == IconStatusRole)
    {
        if(index.column() == SystemColumn)
            return item->isSystem();

        else if(index.column() == FavouriteColumn)
            return item->isFavourite();

        return QVariant();
    }

    else if(role == Qt::ToolTipRole)
    {
        if(item->isSystem() && (index.column() == SystemColumn || index.column() == NameColumn ||
          index.column() == HostColumn || index.column() == PortColumn))
        {
            return "This server appears in the central <b>system server list</b> and its name, \
                    host or port cannot be modified.";
        }

        return QString();
    }

	return QVariant();
}

QVariant ServerListModel::headerData(int section,Qt::Orientation ori,int role) const
{
	if(ori != Qt::Horizontal)
	{
		return QVariant();
	}

	if(role == Qt::DisplayRole)
	{
		switch(section)
		{
    		case LoadColumn: return tr("L");
    		case NameColumn: return tr("Name");
    		case HostColumn: return tr("Host");
            case PortColumn: return tr("Port");
            case SystemColumn: return tr("S");
            case FavouriteColumn: return tr("F");
            case UseColumn: return tr("Loaded");
    		default: return QVariant();
		}
	}
	else if(role == Qt::ToolTipRole)
	{
		switch(section)
		{
    		case LoadColumn: return tr("Indicates if the server is <b>loaded</b> in the <b>current</b> tab");
    		case NameColumn: return tr("Server name is a freely customisable <b>nickname</b>. It is only used by the </b>viewer</b>.");
    		case HostColumn: return tr("Hostname of the server");
    		case PortColumn: return tr("Port number of the server");
            case SystemColumn: return tr("Indicates if a server appears in the centrally maintained <b>system server list</b>. \
                                         <br>The name, host and port of these server entries cannot be edited.");
            case FavouriteColumn: return tr("Indicates if a server is a <b>favourite</b>. Only favourite and loaded servers \
    				                        are appearing in the server list under the <b>Servers menu</b> in the menubar");
    		case UseColumn: return tr("Indicates the <b>number of tabs</b> where the server is loaded.");
    		default: return QVariant();
		}
	}
	else if(role == Qt::TextAlignmentRole)
	{
		return Qt::AlignCenter;
	}



    return QVariant();
}

bool ServerListModel::setData(const QModelIndex& idx, const QVariant & value, int role )
{
	if(filter_ && idx.column() == LoadColumn && role == Qt::CheckStateRole)
	{
		if(ServerItem* item=ServerList::instance()->itemAt(idx.row()))
		{
			bool checked=(value.toInt() == Qt::Checked)?true:false;
			if(checked)
				filter_->addServer(item);
			else
				filter_->removeServer(item);

			QModelIndex startIdx=index(idx.row(),0);
			QModelIndex endIdx=index(idx.row(),columnCount()-1);

			Q_EMIT dataChanged(startIdx,endIdx);
			return true;
		}
	}

	return false;
}


QModelIndex ServerListModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
	return createIndex(row,column,static_cast<void*>(nullptr));
}

QModelIndex ServerListModel::parent(const QModelIndex &) const
{
	return {};
}

Qt::ItemFlags ServerListModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if(filter_ && index.column() == LoadColumn)
	{
		defaultFlags=defaultFlags | Qt::ItemIsUserCheckable;
	}

	return defaultFlags;
}

ServerItem* ServerListModel::indexToServer(const QModelIndex& index)
{
	return ServerList::instance()->itemAt(index.row());
}

//======================================
//
// ServerListFilterModel
//
//======================================

ServerListFilterModel::ServerListFilterModel(QObject *parent) :
	QSortFilterProxyModel(parent)
{

}

void ServerListFilterModel::setFilterStr(QString t)
{
	QString newStr=t.simplified();
	if(newStr != filterStr_)
	{
		filterStr_=newStr;
		invalidateFilter();
	}
}

void ServerListFilterModel::setFilterFavourite(bool b)
{
	if(b != filterFavourite_)
	{
		filterFavourite_=b;
		invalidateFilter();
	}
}


bool ServerListFilterModel::filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
{
	if(filterFavourite_)
	{
		QModelIndex idxLoad = sourceModel()->index(sourceRow, ServerListModel::LoadColumn, sourceParent);
		QModelIndex idxFav = sourceModel()->index(sourceRow, ServerListModel::FavouriteColumn, sourceParent);
		if(!sourceModel()->data(idxFav,Qt::UserRole).toBool() &&
			sourceModel()->data(idxLoad,Qt::CheckStateRole).toInt() != Qt::Checked)
			return false;
	}

	if(filterStr_.isEmpty())
		return true;

	QModelIndex idxName = sourceModel()->index(sourceRow,ServerListModel::NameColumn, sourceParent);
	QModelIndex idxHost= sourceModel()->index(sourceRow,ServerListModel::HostColumn, sourceParent);

	if(sourceModel()->data(idxName).toString().contains(filterStr_,Qt::CaseInsensitive) ||
	   sourceModel()->data(idxHost).toString().contains(filterStr_,Qt::CaseInsensitive))
	return true;

	return false;

}

bool ServerListFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(left.column() == ServerListModel::SystemColumn || left.column() == ServerListModel::FavouriteColumn)
    {
        return left.data(ServerListModel::IconStatusRole).toBool() <
               right.data(ServerListModel::IconStatusRole).toBool();
    }

    return QSortFilterProxyModel::lessThan(left,right);
}



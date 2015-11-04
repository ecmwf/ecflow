//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


#include "NodeQueryWidget.hpp"

#include "ComboMulti.hpp"
#include "CustomListWidget.hpp"
#include "Highlighter.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryEngine.hpp"
#include "NodeQueryHandler.hpp"
#include "ServerFilter.hpp"
#include "VNState.hpp"

#include <QtGlobal>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QPalette>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

//======================================================
//
// NodeQuerySaveDialog
//
//======================================================

NodeQuerySaveDialog::NodeQuerySaveDialog(QWidget *parent) : QDialog(parent)
{
	setupUi(this);
}

QString NodeQuerySaveDialog::name() const
{
	return nameLe_->text();
}

void NodeQuerySaveDialog::accept()
{
	QString name=nameLe_->text();

	if(!name.contains(QRegExp("[\\w|\\s]+")))
	{
		QMessageBox::critical(0,tr("Invalid character"),
				"Query names can only contain alphanumeric characters, whitespaces and \"_\". Please choose a different name.");
		return;

	}

	if(NodeQueryHandler::instance()->find(name.toStdString()))
	{
		QMessageBox::critical(0,tr("Duplicated"),
					"The specified name is already used by another query. Please choose a different name.");
		return;
	}

	QDialog::accept();
}

//======================================================
//
// NodeQueryWidget
//
//======================================================

NodeQueryWidget::NodeQueryWidget(QWidget *parent) :
    QWidget(parent),
	query_(NULL),
	serverFilter_(NULL)
{
    setupUi(this);

//#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
//   nameLe_->setPlaceholderText(tr("Search"));
//#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    nameLe_->setClearButtonEnabled(true);
    pathLe_->setClearButtonEnabled(true);
    rootLe_->setClearButtonEnabled(true);
#endif

    //Query list
    queryListPanel_->hide();
    queryListTb_->setChecked(false);

    connect(queryListTb_,SIGNAL(clicked(bool)),
    		queryListPanel_,SLOT(setVisible(bool)));

	//Query
	QFont f;
	QFontMetrics fm(f);

	queryTe_->setFixedHeight((fm.height()+2)*3+6);
	queryTe_->setReadOnly(true);

    Highlighter* ih=new Highlighter(queryTe_->document(),"query");

    //Max item num
    numSpin_->setRange(1,500000);
    numSpin_->setValue(50000);
    numCh_->setChecked(true);
    connect(numCh_,SIGNAL(clicked(bool)),
    		numSpin_,SLOT(setEnabled(bool)));

    //Servers
    serverResetTb_->setEnabled(serverCb_->hasSelection());

    //Node type
    QStringList typeLst;
    typeLst << "server" << "suite" << "family" << "task" << "alias";
    typeList_->addItems(typeLst,false);

    //Node state
    QStringList stateLst;
    stateLst << "aborted" << "active" << "complete" << "queued" << "submitted" << "suspended" << "unknown";
    QList<QColor> stateColLst;
    Q_FOREACH(QString s,stateLst)
    {
    	if(VNState* vn=VNState::find(s.toStdString()))
    	{
    		stateColLst << vn->colour();
    	}
    	else
    	{
    		stateColLst << QColor();
    	}
    }

    stateList_->addItems(stateLst,false,stateColLst);

    //Node flags
    QStringList flagLst;
    flagLst << "is_late" << "has_date" << "has_message" << "has_time" << "is_rerun" << "is_waiting" << "is_zombie";
    flagList_->addItems(flagLst,false);

    //Attributes
    attrList_->addItems(attrPanel_->groupNames(),false);

    int listHeight=(fm.height()+2)*7+6;
    typeList_->setFixedHeight(listHeight);
    stateList_->setFixedHeight(listHeight);
    flagList_->setFixedHeight(listHeight);
    attrList_->setFixedHeight(listHeight);

    typeResetTb_->setEnabled(typeList_->hasSelection());
    stateResetTb_->setEnabled(stateList_->hasSelection());
    flagResetTb_->setEnabled(flagList_->hasSelection());
    attrResetTb_->setEnabled(attrList_->hasSelection());


    connect(exactMatchCh_,SIGNAL(clicked(bool)),
    		this,SLOT(slotExactMatch(bool)));

    connect(nameLe_,SIGNAL(textEdited(QString)),
        	this,SLOT(slotSearchTermEdited(QString)));

    connect(pathLe_,SIGNAL(textEdited(QString)),
           	this,SLOT(slotSearchTermEdited(QString)));

    connect(rootLe_,SIGNAL(textEdited(QString)),
            this,SLOT(slotSearchTermEdited(QString)));

    connect(serverCb_,SIGNAL(selectionChanged()),
            this,SLOT(slotServerCbChanged()));

    connect(serverResetTb_,SIGNAL(clicked()),
          	serverCb_,SLOT(clearSelection()));

    //Lists
    connect(typeList_,SIGNAL(selectionChanged()),
           this,SLOT(slotTypeListChanged()));

    connect(stateList_,SIGNAL(selectionChanged()),
           this,SLOT(slotStateListChanged()));

    connect(flagList_,SIGNAL(selectionChanged()),
           this,SLOT(slotFlagListChanged()));

    connect(attrList_,SIGNAL(selectionChanged()),
               this,SLOT(slotAttrListChanged()));

    // List reset buttons
    connect(stateResetTb_,SIGNAL(clicked()),
    		stateList_,SLOT(clearSelection()));

    connect(typeResetTb_,SIGNAL(clicked()),
        	typeList_,SLOT(clearSelection()));

    connect(flagResetTb_,SIGNAL(clicked()),
            flagList_,SLOT(clearSelection()));

    connect(attrResetTb_,SIGNAL(clicked()),
            attrList_,SLOT(clearSelection()));

    //attributes
    connect(attrPanel_,SIGNAL(queryChanged()),
    		this,SLOT(buildQueryString()));

    //Show hide def panel
    defPanelTb_->setText("Definitions <<");
    connect(defPanelTb_,SIGNAL(clicked(bool)),
           this,SLOT(slotShowDefPanel(bool)));

    //Find button
    findPb_->setProperty("startSearch","1");
	QPalette pal=findPb_->palette();
	QColor col(230,245,253);
    pal.setColor(QPalette::Button,col);
    findPb_->setPalette(pal);

    connect(findPb_,SIGNAL(clicked()),
    		this,SLOT(slotFind()));

    //Close button
    connect(closePb_,SIGNAL(clicked()),
        	this,SIGNAL(closeClicked()));

    check();

    //--------------------------------
    // Result tree/model
    //--------------------------------

    model_=new NodeQueryModel(this);
    sortModel_=new QSortFilterProxyModel(this);
    sortModel_->setSourceModel(model_);

    resTree_->setRootIsDecorated(false);
    resTree_->setSortingEnabled(true);
    resTree_->setUniformRowHeights(true);
    resTree_->setModel(sortModel_);

    //--------------------------------
    // Query
    //--------------------------------

    query_=new NodeQuery("tmp");

    //--------------------
    // Query engine
    //--------------------

    engine_=new NodeQueryEngine(this);

    connect(engine_,SIGNAL(found(QStringList)),
    		model_, SLOT(appendRow(QStringList)));

    connect(engine_,SIGNAL(started()),
    		this,SLOT(slotQueryStarted()));

    connect(engine_,SIGNAL(finished()),
    		this,SLOT(slotQueryFinished()));

    //-------------------
    // Progress
    //-------------------

    queryProgress_->hide();

    connect(saveAsTb_,SIGNAL(clicked()),
    		this,SLOT(slotSaveQueryAs()));

    //-------------------
    // Query list
    //-------------------

    queryListModel_=new NodeQueryListModel(this);
    queryList_->setModel(queryListModel_);
}

NodeQueryWidget::~NodeQueryWidget()
{
	delete query_;

	if(serverFilter_)
		serverFilter_->removeObserver(this);
}

void NodeQueryWidget::slotShowDefPanel(bool)
{
	defPanel_->setVisible(!defPanel_->isVisible());

	if(defPanel_->isVisible())
	{
		defPanelTb_->setText("Definitions <<");
	}
	else
	{
		defPanelTb_->setText("Definitions >>");
	}
}

void NodeQueryWidget::slotExactMatch(bool)
{
	buildQueryString();
}

void NodeQueryWidget::slotSearchTermEdited(QString)
{
	buildQueryString();
	check();
}

void NodeQueryWidget::slotRootNodeEdited(QString)
{
	buildQueryString();
	check();
}

void NodeQueryWidget::slotServerCbChanged()
{
	serverResetTb_->setEnabled(serverCb_->hasSelection());
	check();
}

void NodeQueryWidget::slotTypeListChanged()
{
	typeResetTb_->setEnabled(typeList_->hasSelection());
	buildQueryString();
	check();
}

void NodeQueryWidget::slotStateListChanged()
{
	stateResetTb_->setEnabled(stateList_->hasSelection());
	buildQueryString();
	check();
}

void NodeQueryWidget::slotFlagListChanged()
{
	flagResetTb_->setEnabled(flagList_->hasSelection());
	buildQueryString();
	check();
}

void NodeQueryWidget::slotAttrListChanged()
{
	attrResetTb_->setEnabled(attrList_->hasSelection());
	attrPanel_->setSelection(attrList_->selection());
	//filterBox_->layout()->invalidate();

	//buildQueryString();
	//check();
}

void NodeQueryWidget::check()
{
	bool oneServer=(serverCb_->selection().count() == 1);

	rootLabel_->setEnabled(oneServer);
	rootLe_->setEnabled(oneServer);

	QString queryStr=queryTe_->toPlainText().simplified();

	bool canSearch=(!queryStr.isEmpty() ||
	  (rootLe_->isEnabled() && !rootLe_->text().simplified().isEmpty()));

	findPb_->setEnabled(canSearch);

	/*if(!searchLe_->text().simplified().isEmpty() &&
	   inCb_->selection().isEmpty())
	{
		findPb_->setEnabled(false);
	}
	else
	{
		findPb_->setEnabled(true);
	}*/
}

void NodeQueryWidget::buildQueryString()
{
	QString s;
	/*QString nameTerm; //=searchLe_->text().simplified();
	if(sTerm.isEmpty())
	{
		sTerm="ANY";
	}
	else
	{
		sTerm="\'" +  sTerm + "\'";
 	}*/
	//Node name
	QString namePart;
	QString name=nameLe_->text().simplified();
	QString path=pathLe_->text().simplified();
	if(!name.isEmpty())
	{
		namePart="node_name = \'" +  name + "\'";
	}
	if(!path.isEmpty())
	{
		if(!namePart.isEmpty())
			namePart+=" or ";

		namePart+="node_path = \'" +  path + "\'";
	}

	if(!namePart.isEmpty())
	{
		s="(" + namePart + ")";
	}

	if(typeList_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + typeList_->selection().join(" or ") + " )";
	}

	if(stateList_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + stateList_->selection().join(" or ") + " )";
	}

	if(flagList_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + flagList_->selection().join(" or ") + " )";
	}

	//Attributes
	QString attrPart;
	QString attr=attrPanel_->query();
	if(!attr.isEmpty())
	{
		attrPart="(" + attr + ")";
		s+=" and " + attrPart;
	}

	queryTe_->setPlainText(s);
}

//------------------------------------------
// Servers
//------------------------------------------

void NodeQueryWidget::updateServers()
{
	serverCb_->clear();

	if(serverFilter_)
	{
		for(std::vector<ServerItem*>::const_iterator it=serverFilter_->items().begin(); it != serverFilter_->items().end(); ++it)
		{
			serverCb_->addItem(QString::fromStdString((*it)->name()));
		}
	}

	serverCb_->selectSoleItem();

	buildQueryString();
}


void NodeQueryWidget::setServerFilter(ServerFilter* sf)
{
	if(serverFilter_ == sf)
		return;

	if(serverFilter_)
	{
		serverFilter_->removeObserver(this);
	}

	serverFilter_=sf;

	if(serverFilter_)
	{
		serverFilter_->addObserver(this);
	}

	updateServers();
}

void NodeQueryWidget::notifyServerFilterAdded(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->addServerObserver(this);
	updateTitle();*/
}

void NodeQueryWidget::notifyServerFilterRemoved(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->removeServerObserver(this);
	updateTitle();*/
}

void NodeQueryWidget::notifyServerFilterChanged(ServerItem*)
{
	//updateTitle();
}

void NodeQueryWidget::notifyServerFilterDelete()
{
	serverFilter_->removeObserver(this);
	serverFilter_=0;
}

//------------------------------------------
// Execute query
//------------------------------------------

void NodeQueryWidget::updateQuery()
{
	query_->setQuery(queryTe_->toPlainText().toStdString());

	//options
	NodeQueryOptions opt;
	opt.exactMatch_=exactMatchCh_->isChecked();
	opt.caseSensitive_=caseCh_->isChecked();
	opt.wildcard_=wildcardCh_->isChecked();
	opt.regexp_=regexpCh_->isChecked();

	if(numSpin_->isEnabled())
		opt.maxNum_=numSpin_->value();

	query_->setOptions(opt);

	//Server list
	std::vector<std::string> servers;
	if(serverCb_->selection().count() > 0)
	{
		Q_FOREACH(QString s,serverCb_->selection())
			servers.push_back(s.toStdString());
	}
	else
	{
		for(int i=0; i < serverCb_->count(); i++)
			servers.push_back(serverCb_->itemText(i).toStdString());
	}

	query_->setServers(servers);

	//RootNode
	std::string rootNode;
	if(rootLe_->isEnabled())
	{
		rootNode=rootLe_->text().simplified().toStdString();
		query_->setRootNode(rootNode);
	}

}

void NodeQueryWidget::slotFind()
{
	//Clear the results
	model_->clearData();

	updateQuery();

	assert(!engine_->isRunning());

	engine_->exec(query_);
}

void NodeQueryWidget::slotStop()
{
	//if(engine_)
	//	engine_->abortFind();
}

void NodeQueryWidget::slotAddResult(QStringList res)
{
	model_->appendRow(res);
}

void NodeQueryWidget::slotQueryStarted()
{
 	qDebug() << "STARTED";
  	findPb_->setEnabled(false);
  	stopPb_->setEnabled(true);

	queryProgress_->setRange(0,0);
	queryProgress_->show();

	progressLabel_->setText("Search in progress ...");
}

void NodeQueryWidget::slotQueryFinished()
{
 	qDebug() << "FINISHED";

	findPb_->setEnabled(true);
  	stopPb_->setEnabled(false);

	queryProgress_->hide();
	queryProgress_->setRange(0,1);
	queryProgress_->setValue(1);

	progressLabel_->setText(QString::number(model_->rowCount()) + " items found");
}

//------------------------------------------
// Query management
//------------------------------------------

void NodeQueryWidget::slotSaveQueryAs()
{
	NodeQuerySaveDialog d(this);
	if(d.exec() == QDialog::Accepted)
	{
		updateQuery();
		std::string name=d.name().toStdString();
		NodeQuery* n=query_->clone(name);
		NodeQueryHandler::instance()->add(n,true);
	}
}

NodeQueryModel::NodeQueryModel(QObject *parent) :
     QAbstractItemModel(parent)
{
}

NodeQueryModel::~NodeQueryModel()
{
}

void NodeQueryModel::clearData()
{
	beginResetModel();
	data_.clear();
	endResetModel();
}

void NodeQueryModel::appendRow(QStringList lst)
{
	const int cnt=data_.count();
	beginInsertRows(QModelIndex(),cnt,cnt);
	data_ << lst;
	endInsertRows();
}

int NodeQueryModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int NodeQueryModel::rowCount( const QModelIndex& parent) const
{
	return (!parent.isValid())?data_.count():0;
}

QVariant NodeQueryModel::data( const QModelIndex& index, int role ) const
{
	if(role != Qt::DisplayRole)
		return QVariant();

	if(!index.isValid())
    {
		return QVariant();
	}

	int row=index.row();
	if(row < 0 || row >= data_.count())
		return QVariant();

	int column=index.column();
	if(column < data_.at(row).count())
		return data_.at(row).at(column);

	return QVariant();
}

QVariant NodeQueryModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
      		  return QAbstractItemModel::headerData( section, orient, role );

   	if(role == Qt::DisplayRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Server");
   		case 1: return tr("Node path");
   		case 2: return tr("Type");
   		default: return QVariant();
   		}
   	}
   	else if(role== Qt::ToolTipRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Server");
   		case 1: return tr("Node");
   		case 2: return tr("Type");
   		default: return QVariant();
   		}
   	}
    return QVariant();
}

QModelIndex NodeQueryModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex NodeQueryModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

NodeQueryListModel::NodeQueryListModel(QObject *parent) :
     QAbstractItemModel(parent)
{
	data_=NodeQueryHandler::instance()->items();
}

NodeQueryListModel::~NodeQueryListModel()
{
}

void NodeQueryListModel::updateData()
{
	beginResetModel();
	data_.clear();
	data_=NodeQueryHandler::instance()->items();
	endResetModel();
}

int NodeQueryListModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 1;
}

int NodeQueryListModel::rowCount( const QModelIndex& parent) const
{
	return (!parent.isValid())?data_.size():0;
}

QVariant NodeQueryListModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid())
	{
		return QVariant();
	}

	if(role != Qt::DisplayRole && role != Qt::ToolTipRole)
		return QVariant();

	int row=index.row();
	if(row < 0 || row >= data_.size())
		return QVariant();

	if(role == Qt::DisplayRole)
		return QString::fromStdString(data_.at(row)->name());
	else if(role == Qt::ToolTipRole)
	{
		QString q=QString::fromStdString(data_.at(row)->query());
		return "<b>query</b>: " + q;

	}
}

QVariant NodeQueryListModel::headerData(const int section, const Qt::Orientation orient , const int role ) const
{
	return QVariant();
}

QModelIndex NodeQueryListModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex NodeQueryListModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}










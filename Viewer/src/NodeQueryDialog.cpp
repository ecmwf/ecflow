
#include "NodeQueryDialog.hpp"

#include "ComboMulti.hpp"
#include "Highlighter.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryEngine.hpp"
#include "NodeQueryHandler.hpp"
#include "ServerFilter.hpp"

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
// NodeQueryDialog
//
//======================================================

NodeQueryDialog::NodeQueryDialog(QWidget *parent) :
    QDialog(parent),
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
    numSpin_->setRange(1,100000);
    numSpin_->setValue(10000);
    numCh_->setChecked(true);
    connect(numCh_,SIGNAL(clicked(bool)),
    		numSpin_,SLOT(setEnabled(bool)));

    //Node type
    typeCb_->addItem("server");
    typeCb_->addItem("suite");
    typeCb_->addItem("family");
    typeCb_->addItem("task");
    typeCb_->addItem("alias");

    //Node state
    stateCb_->addItem("aborted");
    stateCb_->addItem("active");
    stateCb_->addItem("complete");
    stateCb_->addItem("queued");
    stateCb_->addItem("submitted");
    stateCb_->addItem("suspended");
    stateCb_->addItem("unknown");

    //Flags
    flagCb_->addItem("is_late");
    flagCb_->addItem("has_date");
    flagCb_->addItem("has_message");
    flagCb_->addItem("has_time");
    flagCb_->addItem("is_rerun");
    flagCb_->addItem("is_waiting");
    flagCb_->addItem("is_zombie");


    connect(exactMatchCh_,SIGNAL(clicked(bool)),
    		this,SLOT(slotExactMatch(bool)));

    connect(nameLe_,SIGNAL(textEdited(QString)),
        	this,SLOT(slotSearchTermEdited(QString)));

    connect(pathLe_,SIGNAL(textEdited(QString)),
           	this,SLOT(slotSearchTermEdited(QString)));

    connect(rootLe_,SIGNAL(textEdited(QString)),
            this,SLOT(slotSearchTermEdited(QString)));

    connect(serverCb_,SIGNAL(selectionChanged()),
               this,SLOT(check()));

    connect(stateCb_,SIGNAL(selectionChanged()),
            this,SLOT(slotStateCbChanged()));

    connect(typeCb_,SIGNAL(selectionChanged()),
    		this,SLOT(slotTypeCbChanged()));

    connect(flagCb_,SIGNAL(selectionChanged()),
            this,SLOT(slotFlagCbChanged()));

    //Reset buttons
    connect(stateResetTb_,SIGNAL(clicked()),
    		stateCb_,SLOT(clearSelection()));

    connect(typeResetTb_,SIGNAL(clicked()),
        	typeCb_,SLOT(clearSelection()));

    connect(flagResetTb_,SIGNAL(clicked()),
            flagCb_,SLOT(clearSelection()));

    //attributes
    connect(attrW_,SIGNAL(queryChanged()),
    		this,SLOT(buildQueryString()));

    //Find button
    findPb_->setProperty("startSearch","1");
	QPalette pal=findPb_->palette();
	QColor col(230,245,253);
    pal.setColor(QPalette::Button,col);
    findPb_->setPalette(pal);

    connect(findPb_,SIGNAL(clicked()),
    		this,SLOT(slotFind()));

    tab_->setCurrentIndex(0);

    tab_->setFixedHeight(190);

    //Read the qt settings
    readSettings();

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

NodeQueryDialog::~NodeQueryDialog()
{
	delete query_;

	if(serverFilter_)
		serverFilter_->removeObserver(this);
}


void NodeQueryDialog::closeEvent(QCloseEvent * event)
{
	event->accept();
	writeSettings();
}


void NodeQueryDialog::accept()
{
	writeSettings();
    QDialog::accept();
}


void NodeQueryDialog::reject()
{
	writeSettings();
	QDialog::reject();
}

void NodeQueryDialog::slotExactMatch(bool)
{
	buildQueryString();
}

void NodeQueryDialog::slotSearchTermEdited(QString)
{
	buildQueryString();
	check();
}

void NodeQueryDialog::slotRootNodeEdited(QString)
{
	buildQueryString();
	check();
}

void NodeQueryDialog::slotTypeCbChanged()
{
	buildQueryString();
	check();
}

void NodeQueryDialog::slotStateCbChanged()
{
	buildQueryString();
	check();
}

void NodeQueryDialog::slotFlagCbChanged()
{
	buildQueryString();
	check();
}

void NodeQueryDialog::check()
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

void NodeQueryDialog::buildQueryString()
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

	if(stateCb_->selection().count() >0)
	{
		s+="( " + stateCb_->selection().join(" or ") + " )";
	}

	if(typeCb_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + typeCb_->selection().join(" or ") + " )";
	}

	if(flagCb_->selection().count() >0)
	{
		if(!s.isEmpty())
			s+=" and ";
		s+="( " + flagCb_->selection().join(" or ") + " )";
	}

	QString eqPart;

	//Node name
	QString name=nameLe_->text().simplified();
	if(!name.isEmpty())
	{
		eqPart="node_name = \'" +  name + "\'";
	}

	QString path=pathLe_->text().simplified();
	if(!path.isEmpty())
	{
		if(!eqPart.isEmpty())
			eqPart+=" or ";
		eqPart+="node_path = \'" +  path + "\'";
	}

	//Attributes
	QString attr=attrW_->query();
	if(!attr.isEmpty())
	{
		if(!eqPart.isEmpty())
			eqPart+=" or ";
		eqPart+=attr;
	}

	if(!eqPart.isEmpty())
	{
		if(!s.isEmpty())
			s+=" and (" + eqPart + ")";
		else
			s=eqPart;
	}

	queryTe_->setPlainText(s);
}

//------------------------------------------
// Servers
//------------------------------------------

void NodeQueryDialog::updateServers()
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


void NodeQueryDialog::setServerFilter(ServerFilter* sf)
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

void NodeQueryDialog::notifyServerFilterAdded(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->addServerObserver(this);
	updateTitle();*/
}

void NodeQueryDialog::notifyServerFilterRemoved(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->removeServerObserver(this);
	updateTitle();*/
}

void NodeQueryDialog::notifyServerFilterChanged(ServerItem*)
{
	//updateTitle();
}

void NodeQueryDialog::notifyServerFilterDelete()
{
	serverFilter_->removeObserver(this);
	serverFilter_=0;
}

//------------------------------------------
// Execute query
//------------------------------------------

void NodeQueryDialog::updateQuery()
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

void NodeQueryDialog::slotFind()
{
	//Clear the results
	model_->clearData();

	updateQuery();

	assert(!engine_->isRunning());

	engine_->exec(query_);
}

void NodeQueryDialog::slotStop()
{
	//if(engine_)
	//	engine_->abortFind();
}

void NodeQueryDialog::slotAddResult(QStringList res)
{
	model_->appendRow(res);
}

void NodeQueryDialog::slotQueryStarted()
{
 	qDebug() << "STARTED";
  	findPb_->setEnabled(false);
  	stopPb_->setEnabled(true);

	queryProgress_->setRange(0,0);
	queryProgress_->show();

	progressLabel_->setText("Search in progress ...");
}

void NodeQueryDialog::slotQueryFinished()
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

void NodeQueryDialog::slotSaveQueryAs()
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

//------------------------------------------
// Settings read/write
//------------------------------------------

void NodeQueryDialog::writeSettings()
{
	QSettings settings("ECMWF","ecflowUI-NodeSearchDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	//settings.setValue("current",list_->currentRow());
	settings.endGroup();
}

void NodeQueryDialog::readSettings()
{
	QSettings settings("ECMWF","ecflowUI-NodeSearchDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(550,540));
	}

	/*if(settings.contains("current"))
	{
		int current=settings.value("current").toInt();
		if(current >=0)
			list_->setCurrentRow(current);
	}*/
	settings.endGroup();
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








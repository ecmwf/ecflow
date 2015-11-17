//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


#include "NodeQueryEditor.hpp"

#include "ComboMulti.hpp"
#include "CustomListWidget.hpp"
#include "Highlighter.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryHandler.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

#include <QtGlobal>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QPalette>
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
// NodeQueryEditor
//
//======================================================

NodeQueryEditor::NodeQueryEditor(QWidget *parent) :
    QWidget(parent),
	query_(NULL),
	serverFilter_(NULL),
	queryTeCanExpand_(false),
	initIsOn_(false),
	canBeRun_(false)
{
    setupUi(this);

    query_=new NodeQuery("tmp");
    attrPanel_->setQuery(query_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    nameLe_->setClearButtonEnabled(true);
    pathLe_->setClearButtonEnabled(true);
    rootLe_->setClearButtonEnabled(true);
#endif

	//-------------------------
    // Query display
	//-------------------------

    QFont f;
	QFontMetrics fm(f);

	queryTe_->setFixedHeight((fm.height()+2)*3+6);
	queryTe_->setReadOnly(true);

    Highlighter* ih=new Highlighter(queryTe_->document(),"query");

    //------------------
    // Options
    //------------------

    //Max item num
    numSpin_->setRange(1,500000);
    numSpin_->setValue(50000);
    numCh_->setChecked(true);
    connect(numCh_,SIGNAL(clicked(bool)),
    		numSpin_,SLOT(setEnabled(bool)));

    //-------------------------
    // Scope
    //-------------------------

    //Servers
    serverResetTb_->setEnabled(serverCb_->hasSelection());

    connect(serverCb_,SIGNAL(selectionChanged()),
           this,SLOT(slotServerCbChanged()));

    connect(serverResetTb_,SIGNAL(clicked()),
            serverCb_,SLOT(clearSelection()));

    //Root
    connect(rootLe_,SIGNAL(textEdited(QString)),
           this,SLOT(slotRootNodeEdited(QString)));

    //Name
    connect(nameLe_,SIGNAL(textEdited(QString)),
           this,SLOT(slotNameEdited(QString)));

    connect(nameMatchCb_,SIGNAL(currentIndexChanged(int)),
           this,SLOT(slotNameMatchChanged(int)));

    connect(nameCaseTb_,SIGNAL(changed(bool)),
           this,SLOT(slotNameCaseChanged(bool)));

    //Path
    connect(pathLe_,SIGNAL(textEdited(QString)),
           this,SLOT(slotPathEdited(QString)));

    connect(pathMatchCb_,SIGNAL(currentIndexChanged(int)),
           this,SLOT(slotPathMatchChanged(int)));

    connect(pathCaseTb_,SIGNAL(changed(bool)),
           this,SLOT(slotPathCaseChanged(bool)));

    //-------------------------
    // Filter
    //-------------------------

    //Node type
    typeList_->addItems(NodeQuery::typeTerms(),false);

    connect(typeList_,SIGNAL(selectionChanged()),
            this,SLOT(slotTypeListChanged()));

    connect(typeResetTb_,SIGNAL(clicked()),
          	typeList_,SLOT(clearSelection()));

    //Node state
    stateList_->addItems(NodeQuery::stateTerms(),false); //,stateColLst);

    connect(stateList_,SIGNAL(selectionChanged()),
           this,SLOT(slotStateListChanged()));

    connect(stateResetTb_,SIGNAL(clicked()),
          	stateList_,SLOT(clearSelection()));

    /*QStringList stateLst;
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

    stateList_->addItems(stateLst,false); //,stateColLst);*/

    //Node flags
    flagList_->addItems(NodeQuery::flagTerms(),false);

    connect(flagList_,SIGNAL(selectionChanged()),
           this,SLOT(slotFlagListChanged()));

    connect(flagResetTb_,SIGNAL(clicked()),
            flagList_,SLOT(clearSelection()));

    //Attributes
    attrList_->addItems(NodeQuery::attrGroupTerms(),false);

    connect(attrList_,SIGNAL(selectionChanged()),
           this,SLOT(slotAttrListChanged()));

    connect(attrResetTb_,SIGNAL(clicked()),
            attrList_,SLOT(clearSelection()));

    int listHeight=(fm.height()+2)*7+6;
    typeList_->setFixedHeight(listHeight);
    stateList_->setFixedHeight(listHeight);
    flagList_->setFixedHeight(listHeight);
    attrList_->setFixedHeight(listHeight);

    typeResetTb_->setEnabled(typeList_->hasSelection());
    stateResetTb_->setEnabled(stateList_->hasSelection());
    flagResetTb_->setEnabled(flagList_->hasSelection());
    attrResetTb_->setEnabled(attrList_->hasSelection());

    //--------------------------------
    // Query management
    //--------------------------------

    connect(saveAsTb_,SIGNAL(clicked()),
    		this,SLOT(slotSaveQueryAs()));


    connect(advModeTb_,SIGNAL(clicked(bool)),
       		this,SLOT(slotAdvMode(bool)));

    advModeTb_->hide();

    numSpin_->hide();
    numCh_->hide();

    checkGuiState();
}

NodeQueryEditor::~NodeQueryEditor()
{
	delete query_;

	if(serverFilter_)
		serverFilter_->removeObserver(this);
}

void NodeQueryEditor::init()
{
	initIsOn_=true;

	//Servers
	QStringList servers=query_->servers();
	if(servers == serverCb_->all())
		serverCb_->clearSelection();
	else
		serverCb_->setSelection(servers);

	//Node name
	NodeQueryStringOption* op=query_->stringOption("node_name");
	assert(op);
	nameLe_->setText(op->value());
	nameMatchCb_->setMatchMode(op->matchMode());
	nameCaseTb_->setChecked(op->caseSensitive());

	//Node path
	op=query_->stringOption("node_path");
	assert(op);
	pathLe_->setText(op->value());
	pathMatchCb_->setMatchMode(op->matchMode());
	pathCaseTb_->setChecked(op->caseSensitive());

	//Lists
	typeList_->setSelection(query_->typeSelection());
	stateList_->setSelection(query_->stateSelection());
	flagList_->setSelection(query_->flagSelection());
	attrList_->setSelection(query_->attrGroupSelection());

	attrPanel_->init();

	initIsOn_=false;

	checkGuiState();

}

void NodeQueryEditor::setQueryTeCanExpand(bool b)
{
	queryTeCanExpand_=b;
	if(queryTeCanExpand_)
	{
		queryTe_->setFixedHeight(QWIDGETSIZE_MAX);
	}
	else
	{
		adjustQueryTe();
	}
}

void NodeQueryEditor::toggleDefPanelVisible()
{
	bool b=isDefPanelVisible();
	scopeBox_->setVisible(!b);
	filterBox_->setVisible(!b);
}

bool NodeQueryEditor::isDefPanelVisible() const
{
	return scopeBox_->isVisible();
}

void NodeQueryEditor::slotAdvMode(bool b)
{
	filterBox_->setVisible(!b);
	queryTe_->setReadOnly(!b);

	if(b)
	{
		adjustQueryTe(6);
	}
	else
	{
		adjustQueryTe();
	}
}

void NodeQueryEditor::slotServerCbChanged()
{
	serverResetTb_->setEnabled(serverCb_->hasSelection());

	if(!initIsOn_)
	{
		//Server list
		if(serverCb_->hasSelection())
		{
			query_->setServers(serverCb_->selection());
		}
		else
		{
			query_->setServers(serverCb_->all());
		}
		checkGuiState();
	}
}

void NodeQueryEditor::slotRootNodeEdited(QString s)
{
	if(!initIsOn_)
	{
		query_->setRootNode(rootLe_->text().simplified().toStdString());
		checkGuiState();
	}
}

void NodeQueryEditor::slotNameEdited(QString val)
{
	if(!initIsOn_)
	{
		NodeQueryStringOption* op=query_->stringOption("node_name");
		assert(op);
		op->setValue(val);
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotNameMatchChanged(int val)
{
	if(!initIsOn_)
	{
		NodeQueryStringOption* op=query_->stringOption("node_name");
		assert(op);
		op->setMatchMode(nameMatchCb_->matchMode(val));
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotNameCaseChanged(bool val)
{
	if(!initIsOn_)
	{
		NodeQueryStringOption* op=query_->stringOption("node_name");
		assert(op);
		op->setCaseSensitive(val);
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotPathEdited(QString val)
{
	if(!initIsOn_)
	{
		NodeQueryStringOption* op=query_->stringOption("node_path");
		assert(op);
		op->setValue(val);
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotPathMatchChanged(int val)
{
	if(!initIsOn_)
	{
		NodeQueryStringOption* op=query_->stringOption("node_path");
		assert(op);
		op->setMatchMode(pathMatchCb_->matchMode(val));
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotPathCaseChanged(bool val)
{
	if(!initIsOn_)
	{
		NodeQueryStringOption* op=query_->stringOption("node_path");
		assert(op);
		op->setCaseSensitive(val);
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotTypeListChanged()
{
	if(!initIsOn_)
	{
		typeResetTb_->setEnabled(typeList_->hasSelection());
		query_->setTypeSelection(typeList_->selection());
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotStateListChanged()
{
	if(!initIsOn_)
	{
		stateResetTb_->setEnabled(stateList_->hasSelection());
		query_->setStateSelection(stateList_->selection());
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotFlagListChanged()
{
	if(!initIsOn_)
	{
		flagResetTb_->setEnabled(flagList_->hasSelection());
		query_->setFlagSelection(flagList_->selection());
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotAttrListChanged()
{
	if(!initIsOn_)
	{
		attrResetTb_->setEnabled(attrList_->hasSelection());
		attrPanel_->setSelection(attrList_->selection());
		query_->setAttrGroupSelection(attrList_->selection());
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::checkGuiState()
{
	serverResetTb_->setEnabled(serverCb_->hasSelection());

	bool oneServer=(serverCb_->selection().count() == 1);

	rootLabel_->setEnabled(oneServer);
	rootLe_->setEnabled(oneServer);

	bool canBeRun=(!query_->queryString().isEmpty() ||
	  (rootLe_->isEnabled() && !rootLe_->text().simplified().isEmpty()));

	if(canBeRun_ != canBeRun)
	{
		canBeRun_=canBeRun;
		Q_EMIT queryEnabledChanged(canBeRun_);
	}
}

void NodeQueryEditor::updateQueryTe()
{
	QString q=query_->queryString();

	//Break into lines
	QStringList lst=q.split(") and ");
	if(lst.count() > 1)
		q=lst.join(") and\n");

	setQueryTe(q);
}

void NodeQueryEditor::setQueryTe(QString s)
{
	int rowNum=s.count("\n")+1;
	if(rowNum==0 && !s.isEmpty())
		rowNum=1;

	int oldRowNum=queryTe_->toPlainText().count("\n")+1;
	if(oldRowNum==0 && !queryTe_->toPlainText().isEmpty())
		oldRowNum=1;

	queryTe_->setPlainText(s);

	if(!queryTeCanExpand_)
	{
		if(oldRowNum != rowNum && oldRowNum > 3 || rowNum > 3)
		{
			QFont f;
			QFontMetrics fm(f);

			queryTe_->setFixedHeight((fm.height()+2)*rowNum+6);
		}
	}
}

void NodeQueryEditor::adjustQueryTe(int rn)
{
	int rowNum=0;
	if(rn <= 0)
	{
		rowNum=queryTe_->toPlainText().count("\n")+1;
	}
	else
	{
		rowNum=rn;
	}

	if(rowNum < 3)
		rowNum=3;

	if(!queryTeCanExpand_)
	{
		QFont f;
		QFontMetrics fm(f);

		queryTe_->setFixedHeight((fm.height()+2)*rowNum+6);
	}
}

//------------------------------------------
// Servers
//------------------------------------------

void NodeQueryEditor::updateServers()
{
	serverCb_->clear();

	if(serverFilter_)
	{
		for(std::vector<ServerItem*>::const_iterator it=serverFilter_->items().begin(); it != serverFilter_->items().end(); ++it)
		{
			serverCb_->addItem(QString::fromStdString((*it)->name()));
		}
	}

	//Init
	if(serverCb_->count() == 1)
	{
		serverCb_->selectSoleItem();
	}
	else
	{
		slotServerCbChanged();
	}

}

void NodeQueryEditor::setServerFilter(ServerFilter* sf)
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

void NodeQueryEditor::notifyServerFilterAdded(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->addServerObserver(this);
	updateTitle();*/
}

void NodeQueryEditor::notifyServerFilterRemoved(ServerItem* item)
{
	/*ServerHandler* s=item->serverHandler();
	s->removeServerObserver(this);
	updateTitle();*/
}

void NodeQueryEditor::notifyServerFilterChanged(ServerItem*)
{
	//updateTitle();
}

void NodeQueryEditor::notifyServerFilterDelete()
{
	serverFilter_->removeObserver(this);
	serverFilter_=0;
}

//------------------------------------------
// Root node
//------------------------------------------

void NodeQueryEditor::setRootNode(VInfo_ptr info)
{
	if(info && info.get())
	{
		if(ServerHandler *server=info->server())
		{
			QStringList sel(QString::fromStdString(server->name()));
			serverCb_->setSelection(sel);

			if(info->isNode())
			{
				if(VNode *node=info->node())
				{
					rootLe_->setText(QString::fromStdString(node->absNodePath()));
				}
			}
			else
			{
				rootLe_->clear();
			}
		}
	}
}


//------------------------------------------
// Query
//------------------------------------------

NodeQuery* NodeQueryEditor::query() const
{
	return query_;
}

//------------------------------------------
// Query management
//------------------------------------------

void NodeQueryEditor::setQuery(NodeQuery* q)
{
	query_->swap(q);
	init();
}


void NodeQueryEditor::slotSaveQueryAs()
{
	NodeQuerySaveDialog d(this);
	if(d.exec() == QDialog::Accepted)
	{
		std::string name=d.name().toStdString();
		NodeQuery* q=query_->clone(name);
		NodeQueryHandler::instance()->add(q,true);
	}
}

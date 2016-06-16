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
#include "NodeQueryOption.hpp"
#include "NodeQueryOptionEdit.hpp"
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
    //attrPanel_->setQuery(query_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    nameLe_->setClearButtonEnabled(true);
    pathLe_->setClearButtonEnabled(true);
    rootLe_->setClearButtonEnabled(true);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
	nameLe_->setPlaceholderText(tr("ANY"));
	pathLe_->setPlaceholderText(tr("ANY"));
#endif

	//-------------------------
    // Query display
	//-------------------------


	//QFont f("Courier");
	/*QFont f("Monospace");
	f.setStyleHint(QFont::TypeWriter);
	f.setFixedPitch(true);
	f.setPointSize(10);
	f.setStyleStrategy(QFont::PreferAntialias);
	queryTe_->setFont(f);*/

	QFont f;
	QFontMetrics fm(f);

	queryTe_->setFixedHeight((fm.height()+2)*3+6);
	queryTe_->setReadOnly(true);
	queryTe_->setWordWrapMode(QTextOption::NoWrap);

    Highlighter* ih=new Highlighter(queryTe_->document(),"query");

    //------------------
    // Options
    //------------------

    //Max item num
    numSpin_->setRange(10,250000);
    numSpin_->setValue(query_->maxNum());
    numSpin_->setToolTip(tr("The maximum possible value is: ") + QString::number(numSpin_->maximum()));

    connect(numSpin_,SIGNAL(valueChanged(int)),
            this,SLOT(slotMaxNum(int)));

    caseCb_->setChecked(query_->caseSensitive());

    connect(caseCb_,SIGNAL(clicked(bool)),
    		this,SLOT(slotCase(bool)));

    //-------------------------
    // Scope
    //-------------------------

    //Servers
    serverCb_->setMode(ComboMulti::FilterMode);

	serverResetTb_->setEnabled(serverCb_->hasSelection());

    connect(serverCb_,SIGNAL(selectionChanged()),
           this,SLOT(slotServerCbChanged()));

    connect(serverResetTb_,SIGNAL(clicked()),
            serverCb_,SLOT(clearSelection()));

    //Root
    connect(rootLe_,SIGNAL(textChanged(QString)),
           this,SLOT(slotRootNodeEdited(QString)));

    //Name
    nameEdit_=new NodeQueryStringOptionEdit(query_->option("node_name"),nodePathGrid_,this);
    pathEdit_=new NodeQueryStringOptionEdit(query_->option("node_path"),nodePathGrid_,this);

    //-------------------------
    // Filter
    //-------------------------

    //Node type
    typeEdit_=new NodeQueryListOptionEdit(query_->option("type"),typeList_,typeResetTb_,this);
    stateEdit_=new NodeQueryListOptionEdit(query_->option("state"),stateList_,stateResetTb_,this);
    flagEdit_=new NodeQueryListOptionEdit(query_->option("flag"),flagList_,flagResetTb_,this);
    attrEdit_=new NodeQueryListOptionEdit(query_->option("attribute"),attrList_,attrResetTb_,this);

    int listHeight=(fm.height()+2)*6+6;
    typeList_->setFixedHeight(listHeight);
    stateList_->setFixedHeight(listHeight);
    flagList_->setFixedHeight(listHeight);
    attrList_->setFixedHeight(listHeight);

    //Attr panel
    //connect(attrPanel_,SIGNAL(queryChanged()),
    //        this,SLOT(slotAttrPanelChanged()));


    //Attributes
    Q_FOREACH(NodeQueryAttrGroup* aGrp,query_->attrGroup().values())
    {
        Q_ASSERT(aGrp);
        QString grName=aGrp->name();

        Q_FOREACH(NodeQueryOption* op,aGrp->options())
        {
            NodeQueryOptionEdit *e=0;
            //TODO: use factory here
            if(op->type() == "string")
                e=new NodeQueryStringOptionEdit(op,attrGrid_,this);
            else if(op->type() == "combo")
                e=new NodeQueryComboOptionEdit(op,attrGrid_,this);

            Q_ASSERT(e);
            attr_[grName] << e;
            e->setVisible(false);
        }
    }

    attrPanel_->hide();

    //--------------------------------
    // Query management
    //--------------------------------

    connect(saveAsTb_,SIGNAL(clicked()),
    		this,SLOT(slotSaveQueryAs()));

    connect(advModeTb_,SIGNAL(clicked(bool)),
       		this,SLOT(slotAdvMode(bool)));

    queryManageW_->hide();

    /*advModeTb_->hide();
    queryCbLabel_->hide();
    queryCb_->hide();
    saveAsTb_->hide();
    saveTb_->hide();*/

    //attrList_->hide();
    //attrLabel_->hide();
    //attrResetTb_->hide();

    init();

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

	numSpin_->setValue(query_->maxNum());
	caseCb_->setChecked(query_->caseSensitive());

	numSpin_->setEnabled(!query_->ignoreMaxNum());
	numLabel_->setEnabled(!query_->ignoreMaxNum());

	//Servers
	QStringList servers=query_->servers();
	if(servers == serverCb_->all())
		serverCb_->clearSelection();
	else
		serverCb_->setSelection(servers);

	//Node name
    nameEdit_->init(query_);
    pathEdit_->init(query_);

	//Lists
    typeEdit_->init(query_);
    stateEdit_->init(query_);
    flagEdit_->init(query_);
    attrEdit_->init(query_);

    //attrPanel_->init();

	initIsOn_=false;

	updateQueryTe();
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
	optionBox_->setVisible(!b);
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

void NodeQueryEditor::slotMaxNum(int v)
{
	if(!initIsOn_)
	{
		query_->setMaxNum(v);
		updateQueryTe();
	}
}

void NodeQueryEditor::slotCase(bool b)
{
	if(!initIsOn_)
	{
		query_->setCaseSensitive(b);
		updateQueryTe();
	}
}

void NodeQueryEditor::slotServerCbChanged()
{
	serverResetTb_->setEnabled(serverCb_->hasSelection());

	if(!initIsOn_)
	{
		query_->setServers(serverCb_->selection());
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotRootNodeEdited(QString s)
{
	if(!initIsOn_)
	{
		query_->setRootNode(rootLe_->text().simplified().toStdString());
		updateQueryTe();
		checkGuiState();
	}
}

void NodeQueryEditor::slotOptionEditChanged()
{
    if(!initIsOn_)
    {      
        NodeQueryOptionEdit *e=static_cast<NodeQueryOptionEdit*>(sender());
        Q_ASSERT(e);
        if(e->optionId() == "attribute")
           setAttributePanel(attrList_->selection());

        updateQueryTe();
        checkGuiState();
    }
}

void NodeQueryEditor::setAttributePanel(QStringList lst)
{
    QMapIterator<QString,QList<NodeQueryOptionEdit*> > it(attr_);
    while (it.hasNext())
    {
        it.next();
        bool st=lst.contains(it.key());
        Q_FOREACH(NodeQueryOptionEdit* e,it.value())
        {
            e->setVisible(st);
        }
    }

    if(lst.isEmpty())
        attrPanel_->hide();
    else
        attrPanel_->show();
}


#if 0
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
#endif


void NodeQueryEditor::slotAttrPanelChanged()
{
    if(!initIsOn_)
    {
        updateQueryTe();
        checkGuiState();
    }
}

void NodeQueryEditor::checkGuiState()
{
	serverResetTb_->setEnabled(serverCb_->hasSelection());

	bool oneServer=(serverCb_->count() == 1 || serverCb_->selection().count() == 1);

	rootLabel_->setEnabled(oneServer);
	rootLe_->setEnabled(oneServer);
}

void NodeQueryEditor::updateQueryTe()
{
	query_->buildQueryString();

	QColor bg(241,241,241);
	setQueryTe(query_->extQueryHtml(true,bg,65));
}

void NodeQueryEditor::setQueryTe(QString s)
{
	int rowNum=s.count("<tr>")+s.count("<br>")+1;
	if(rowNum==0 && !s.isEmpty())
		rowNum=1;

	int oldRowNum=queryTe_->toPlainText().count("\n")+1;
	if(oldRowNum==0 && !queryTe_->toPlainText().isEmpty())
		oldRowNum=1;

	queryTe_->setHtml(s);

	if(!queryTeCanExpand_)
	{
        if(oldRowNum != rowNum && (oldRowNum > 3 || rowNum > 3))
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
		QFontMetrics fm(queryTe_->font());

		queryTe_->setFixedHeight((fm.height()+2)*rowNum+6);
	}
}

//------------------------------------------
// Servers
//------------------------------------------

QStringList NodeQueryEditor::allServers() const
{
	return serverCb_->all();
}

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
		//serverCb_->selectSoleItem();
	}
	else
	{
		//slotServerCbChanged();
	}

	slotServerCbChanged();

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

int NodeQueryEditor::maxNum() const
{
	return numSpin_->value();
}

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

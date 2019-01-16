//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeSearchWidget.hpp"

#include "ComboMulti.hpp"
#include "CustomListWidget.hpp"
#include "Highlighter.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryEngine.hpp"
#include "NodeQueryHandler.hpp"
#include "NodeQueryHandler.hpp"
#include "NodeQueryResult.hpp"
#include "NodeQueryResultModel.hpp"
#include "ServerFilter.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"

#include <QtGlobal>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPalette>
#include <QVBoxLayout>

#define _UI_NODESEARCHWIDGET_DEBUG

//======================================================
//
// NodeQueryWidget
//
//======================================================

NodeSearchWidget::NodeSearchWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    //--------------------
    // Show/hide
    //--------------------

    //Show hide def panel
    connect(defPanelTb_,SIGNAL(toggled(bool)),
           this,SLOT(slotShowDefPanel(bool)));

	editor_->show();

    //connect(editor_,SIGNAL(queryEnabledChanged(bool)),
    //		this,SLOT(slotQueryEnabledChanged(bool)));

    //Show hide query
    connect(queryPanelTb_,SIGNAL(toggled(bool)),
           this,SLOT(slotShowQueryPanel(bool)));

    QFont showf;
    showf.setBold(true);
    showf.setPointSize(showf.pointSize()-1);
    showLabel_->setFont(showf);
    showLabel_->setText("<font color=\'#565656\'>Show:</font>");

    //Find button
    findPb_->setProperty("startSearch","1");
	QPalette pal=findPb_->palette();
	QColor col(230,245,253);
    pal.setColor(QPalette::Button,col);
    findPb_->setPalette(pal);

    connect(findPb_,SIGNAL(clicked()),
    		this,SLOT(slotFind()));

    connect(stopPb_,SIGNAL(clicked()),
        	this,SLOT(slotStop()));

    //Clear
    connect(clearPb_,SIGNAL(clicked()),
            editor_,SLOT(slotClear()));

    //Close button
    connect(closePb_,SIGNAL(clicked()),
        	this,SLOT(slotClose()));

    //--------------------------------
    // Result tree/model
    //--------------------------------

    model_=new NodeQueryResultModel(this);
    resTree_->setSourceModel(model_);

    connect(resTree_,SIGNAL(selectionChanged(VInfo_ptr)),
    		this,SIGNAL(selectionChanged(VInfo_ptr)));

    connect(resTree_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
            this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

    //--------------------------------
    // Query
    //--------------------------------

    query_=new NodeQuery("tmp");

    //--------------------
    // Query engine
    //--------------------

    engine_=new NodeQueryEngine(this);

    connect(engine_,SIGNAL(found(QList<NodeQueryResultTmp_ptr>)),
    		model_->data(), SLOT(add(QList<NodeQueryResultTmp_ptr>)));

    connect(engine_,SIGNAL(found(NodeQueryResultTmp_ptr)),
    	    model_->data(),SLOT(add(NodeQueryResultTmp_ptr)));

    connect(engine_,SIGNAL(started()),
    		this,SLOT(slotQueryStarted()));

    connect(engine_,SIGNAL(finished()),
    		this,SLOT(slotQueryFinished()));

    //-------------------
    // Progress
    //-------------------

    queryProgress_->hide();

    stopPb_->setEnabled(false);
}

NodeSearchWidget::~NodeSearchWidget()
{
	delete query_;
}

void NodeSearchWidget::setServerFilter(ServerFilter* f)
{
	editor_->setServerFilter(f);
}

void NodeSearchWidget::setRootNode(VInfo_ptr info)
{
	editor_->setRootNode(info);
}

void NodeSearchWidget::slotShowDefPanel(bool b)
{
    editor_->showDefPanel(b);
}

void NodeSearchWidget::slotShowQueryPanel(bool b)
{
    editor_->showQueryPanel(b);
}

void NodeSearchWidget::slotQueryEnabledChanged(bool queryEnabled)
{
	//if(!engine_->isRunning())
	//{
#if 0
    UiLog().dbg() << "NodeSearchWidget::slotQueryEnabledChanged -->" <<  std::string((queryEnabled?"true":"false"));
    findPb_->setEnabled(queryEnabled);
#endif
	//}
}

void NodeSearchWidget::slotFind()
{
#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << "NodeSearchWidget::slotFind -->";
#endif

    //Avoid double clicking
    if(!findPb_->isEnabled())
    {
         UiLog().dbg() << "<-- NodeSearchWidget::slotFind - search is already running";
         return;
    }

#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << " isRunning=" << engine_->isRunning();
#endif

	adjustColumns();

	//Clear the results
	model_->clearData();

	assert(!engine_->isRunning());
    assert(findPb_->isEnabled());
    assert(!stopPb_->isEnabled());

    //We set the button state in advance as if the engine were running
    adjustButtonState(true);

    elapsed_.start();
    if(!engine_->runQuery(editor_->query(),editor_->allServers()))
    {
        elapsed_=QTime();

        //if we are here we could not start the query and we need to reset the button state
        adjustButtonState();
    }
#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << "<-- NodeSearchWidget::slotFind";
#endif
}

void NodeSearchWidget::slotStop()
{
    //It is a blocking call!
    engine_->stopQuery();
    assert(!engine_->isRunning());
    adjustButtonState();
}

void NodeSearchWidget::slotClose()
{
	slotStop();
	Q_EMIT closeClicked();
}

void NodeSearchWidget::slotQueryStarted()
{
#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << "NodeSearchWidget::slotQueryStarted -->";
#endif
    adjustButtonState();

    queryProgress_->setRange(0,0);
	queryProgress_->show();

	progressLabel_->setText("Search in progress ...");
#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << "<-- slotQueryStarted";
#endif
}

void NodeSearchWidget::slotQueryFinished()
{
#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << "NodeSearchWidget::slotQueryFinished -->";
    UiLog().dbg() << "  Search finished. Total node scanned: " << engine_->scannedCount();
#endif

    adjustButtonState();

	queryProgress_->hide();
	queryProgress_->setRange(0,1);
	queryProgress_->setValue(1);

    QString s="<b>" + QString::number(model_->rowCount()) + "</b> items found in " +
             QString::number(elapsed_.elapsed()*0.001,'f',1)  + " s";

    QColor col(90,92,92);
    if(engine_->wasMaxReached())
    {
        s+=" (stopped due to <b><u><font color=\'" + col.name() + "\'>maxnum reached!</font></u></b>)";
    }
    else if(engine_->wasStopped())
    {
        s+=" (query was <b><u><font color=\'" + col.name() + "\'>interrupted!</font></u></b>)";
    }
    progressLabel_->setText(s);
    elapsed_=QTime();

#ifdef _UI_NODESEARCHWIDGET_DEBUG
    UiLog().dbg() << " isRunning=" << engine_->isRunning();
    UiLog().dbg() << "<-- NodeSearchWidget::slotQueryFinished";
#endif
}


void NodeSearchWidget::adjustButtonState()
{
    adjustButtonState(engine_->isRunning());
}

void NodeSearchWidget::adjustButtonState(bool engineRunning)
{
    if(engineRunning)
    {
        findPb_->setEnabled(false);
        stopPb_->setEnabled(true);
        editor_->setEnabled(false);
    }
    else
    {
        findPb_->setEnabled(true);
        stopPb_->setEnabled(false);
        editor_->setEnabled(true);
    }

    UiLog().dbg() << "NodeSearchWidget::adjustButtonState -->";
    UiLog().dbg() << " findTb_: " <<  findPb_->isEnabled();
    UiLog().dbg() << "<-- adjustButtonState";
}

void NodeSearchWidget::adjustColumns()
{
	if(!columnsAdjusted_)
	{
		columnsAdjusted_=true;

        //We preset the column width. Setting it dynamically can be expensive
        //for a large number of rows (> 1M)
        QFont f;
        QFontMetrics fm(f);
        resTree_->setColumnWidth(0,fm.width("serverserverserse"));
        resTree_->setColumnWidth(1,fm.width("/suite/family1/family2/longtaskname1"));
        resTree_->setColumnWidth(2,fm.width("suspendedAA"));
        resTree_->setColumnWidth(3,fm.width("familyAA"));
        resTree_->setColumnWidth(4,fm.width("2017-Mar-07 15:45:56AA"));
	}
}

void NodeSearchWidget::writeSettings(QSettings &settings)
{
    settings.setValue("defPanel",editor_->isDefPanelVisible());
    settings.setValue("queryPanel",editor_->isQueryPanelVisible());
    QStringList colW;
    for(int i=0; i < resTree_->model()->columnCount()-1; i++)
        colW << QString::number(resTree_->columnWidth(i));

    settings.setValue("resColumnWidth",colW);
}

void NodeSearchWidget::readSettings(const QSettings &settings)
{
    if(settings.contains("defPanel"))
    {
        defPanelTb_->setChecked(settings.value("defPanel").toBool());
    }
    if(settings.contains("queryPanel"))
    {
        queryPanelTb_->setChecked(settings.value("queryPanel").toBool());
    }
    if(settings.contains("resColumnWidth"))
    {
        QStringList lst=settings.value("resColumnWidth").toStringList();
        for(int i=0; i < lst.count(); i++)
            resTree_->setColumnWidth(i,lst[i].toInt());

        if(lst.count() >= 4)
            columnsAdjusted_=true;
    }
}




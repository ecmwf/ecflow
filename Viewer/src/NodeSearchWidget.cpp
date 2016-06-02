//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

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
#include "UserMessage.hpp"
#include "VNState.hpp"

#include <QtGlobal>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QPalette>
#include <QVBoxLayout>
#include "NodeSearchWidget.hpp"

//======================================================
//
// NodeQueryWidget
//
//======================================================

NodeSearchWidget::NodeSearchWidget(QWidget *parent) :
    QWidget(parent),
	query_(NULL),
	columnsAdjusted_(false)
{
    setupUi(this);

	//Query
	QFont f;
	QFontMetrics fm(f);

    //Show hide def panel
    defPanelTb_->setText("Editor <<");
    connect(defPanelTb_,SIGNAL(clicked(bool)),
           this,SLOT(slotShowDefPanel(bool)));

	editor_->show();

	connect(editor_,SIGNAL(queryEnabledChanged(bool)),
			this,SLOT(slotQueryEnabledChanged(bool)));

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

	resW_->hide();
	editor_->setQueryTeCanExpand(true);
    stopPb_->setEnabled(false);
	//mainLayout_->setStretch(1,0);
	//mainLayout_->setStretch(0,1);
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

void NodeSearchWidget::slotShowDefPanel(bool)
{
	editor_->toggleDefPanelVisible();

	if(editor_->isDefPanelVisible())
	{
		defPanelTb_->setText("Editor <<");
	}
	else
	{
		defPanelTb_->setText("Editor >>");
	}
}

void NodeSearchWidget::slotQueryEnabledChanged(bool queryEnabled)
{
	//if(!engine_->isRunning())
	//{
#if 0
    UserMessage::debug("NodeSearchWidget::slotQueryEnabledChanged -->" + std::string((queryEnabled?"true":"false")));
    findPb_->setEnabled(queryEnabled);
#endif
	//}
}

void NodeSearchWidget::slotFind()
{
    UserMessage::debug("NodeSearchWidget::slotFind -->");

    //Avoid double clicking
    if(!findPb_->isEnabled())
    {
         UserMessage::debug("<-- NodeSearchWidget::slotFind - search is already running");
         return;
    }

    qDebug() << engine_->isRunning();

    if(!resW_->isVisible())
	{
		resW_->show();
		editor_->setQueryTeCanExpand(false);
	}

	adjustColumns();

	//Clear the results
	model_->clearData();

	assert(!engine_->isRunning());
    assert(findPb_->isEnabled());
    assert(!stopPb_->isEnabled());

    //We set the button state in advance as if the engine were running
    adjustButtonState(true);

    if(!engine_->runQuery(editor_->query(),editor_->allServers()))
    {
        //if we are here we could not start the query and we need to reset the button state
        adjustButtonState();
    }

    UserMessage::debug("<-- NodeSearchWidget::slotFind");
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
    UserMessage::debug("NodeSearchWidget::slotQueryStarted -->");

    adjustButtonState();

    queryProgress_->setRange(0,0);
	queryProgress_->show();

	progressLabel_->setText("Search in progress ...");

    UserMessage::debug("<-- NodeSearchWidget::slotQueryStarted");
}

void NodeSearchWidget::slotQueryFinished()
{
    UserMessage::debug("NodeSearchWidget::slotQueryFinished -->");
    UserMessage::debug("  Search finished. Total node scanned: " + boost::lexical_cast<std::string>(engine_->scannedCount()));

    adjustButtonState();

	queryProgress_->hide();
	queryProgress_->setRange(0,1);
	queryProgress_->setValue(1);

	progressLabel_->setText(QString::number(model_->rowCount()) + " items found");

	adjustColumns();

    qDebug() << engine_->isRunning();

    UserMessage::debug("<-- NodeSearchWidget::slotQueryFinished");
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

    UserMessage::debug("NodeSearchWidget::adjustButtonState -->");
    UserMessage::debug(" findTb_: " + boost::lexical_cast<std::string>(findPb_->isEnabled()));
    UserMessage::debug("<-- NodeSearchWidget::adjustButtonState");
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
	resTree_->setColumnWidth(0,fm.width("serverserverser"));
	resTree_->setColumnWidth(1,fm.width("/suite/family1/family2/longtaskname1"));
	resTree_->setColumnWidth(2,fm.width("suspended"));
	resTree_->setColumnWidth(3,fm.width("family"));

	//for(int i=0; i < model_->columnCount()-1; i++)
	//	resTree_->resizeColumnToContents(i);
	}

}





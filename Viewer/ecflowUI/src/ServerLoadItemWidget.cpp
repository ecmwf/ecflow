//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerLoadItemWidget.hpp"

#include <QHBoxLayout>

#include "Node.hpp"

#include "InfoProvider.hpp"

#ifdef ECFLOW_LOGVIEW
#include "LogLoadWidget.hpp"
#endif

#include "ServerHandler.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VNState.hpp"

ServerLoadItemWidget::ServerLoadItemWidget(QWidget *parent)
{
    QHBoxLayout* hb=new QHBoxLayout(this);
    hb->setContentsMargins(0,0,0,0);
#ifdef ECFLOW_LOGVIEW
    w_=new LogLoadWidget(this);
#else
    w_=new QWidget(this);
#endif
    hb->addWidget(w_);

    //We will not keep the contents when the item becomes unselected
    unselectedFlags_.clear();
}

ServerLoadItemWidget::~ServerLoadItemWidget()
{
    clearContents();
}

QWidget* ServerLoadItemWidget::realWidget()
{
    return this;
}

void ServerLoadItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    //set the info. we do not need to observe the node!!!
    info_=info;

    load();
}

void ServerLoadItemWidget::load()
{
#ifdef ECFLOW_LOGVIEW
    if(info_ && info_->server())
    {
        ServerHandler *sh=info_->server();
        Q_ASSERT(sh);
        QString logFile;
        if(VServer* vs=sh->vRoot())
        {
            logFile=QString::fromStdString(vs->findVariable("ECF_LOG",false));

            w_->load(QString::fromStdString(sh->name()),
                         QString::fromStdString(sh->host()),
                         QString::fromStdString(sh->port()),
                         logFile,-50000); //last 50000 rows are read
        }

        //"/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log"
        //w_->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");
    }
#endif
}

void ServerLoadItemWidget::clearContents()
{
#ifdef ECFLOW_LOGVIEW
    w_->clear();
#endif
    InfoPanelItem::clear();
}

void ServerLoadItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //If we are here this item is active but not selected!

        //When it becomes suspended we need to clear everything since the
        //tree is probably cleared at this point
        if(suspended_)
        {
            //textEdit_->clear();
        }
        //When we leave the suspended state we need to reload everything
        else
        {
            load();
        }
    }

    Q_ASSERT(!flags.isSet(SelectedChanged));
}

//The content is independent of the server change
void ServerLoadItemWidget::serverSyncFinished()
{
    if(frozen_)
        return;

    //We do not track changes when the item is not selected
    if(!selected_ || !active_)
        return;

    if(!info_)
        return;

    //For any change we nee to reload
    load();
}

static InfoPanelItemMaker<ServerLoadItemWidget> maker1("server_load");
